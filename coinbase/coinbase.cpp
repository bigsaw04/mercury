/*
 * Copyright (C) 2019 Chris Morrison
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File:   coinbase.cpp
 * Author: Chris Morrison
 *
 * Created on 29 March 2019, 03:10
 */
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <boost/thread/thread.hpp>
#include <tclap/CmdLine.h>
#include <pushbullet.hpp>
#include <coinbase.hpp>
#include <alsa/asoundlib.h>
#include <sndfile.h>

void play_sound(const std::string& sound_file);

static boost::mutex mtx;
static long double fiat_percent = 0;
static std::string work_order_path;
static std::fstream work_order_file;
static std::string coin;
static std::string fiat;
static std::string action;
static communications::messaging::pushbullet message_dispatcher;

void print_change_update(long double up, long double down);
bool execute_trade(cryptocoin::trading::trade_context& context);

int main(int argc, char** argv)
{
    std::vector<std::string> strs;
    std::string next_op;

    // --------------------------------------------------------------------------------------------
    // Get the command line arguments.
    // --------------------------------------------------------------------------------------------

    try
    {
        // Define the command line object, and insert a message
        // that describes the program. The "Command description message"
        // is printed last in the help text. The second argument is the
        // delimiter (usually space) and the last one is the version number.
        // The CmdLine object parses the argv array based on the Arg objects
        // that it contains.
        TCLAP::CmdLine cmd("Automated cryptocoin trading robot", ' ', "0.1");

        // Define a value argument and add it to the command line.
        // A value arg defines a flag and a type of value that it expects,
        // such as "-n Bishop".
        TCLAP::ValueArg<std::string> name_arg("f", "work-order-file", "Full path of the work order file.", true, "", "file path");
        cmd.add(name_arg);
        TCLAP::ValueArg<unsigned int> percent_arg("p", "percent-of-balance", "The percentage of the fiat balance to use for trades (default: 100%)", false, 100, "number");
        cmd.add(percent_arg);

        // Parse the argv array.
        cmd.parse(argc, argv);

        // Get the value parsed by each arg.
        work_order_path = name_arg.getValue();
        unsigned int pc = percent_arg.getValue();
        if (pc < 10)
        {
            std::cerr << "Invalid value for '--percent-of-balance,' at least 10% of your fiat balance must be used." << std::endl;
            return 1;
        }
        fiat_percent = pc / 100.00;

        if (work_order_path.empty() || !std::filesystem::exists(work_order_path))
        {
            std::cerr << "Invalid value for '--work-order-file,' a valid path to an existing file must be given." << std::endl;
            return 1;
        }
    }
    catch (TCLAP::ArgException &e)  // catch any exceptions
    {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    }

    // --------------------------------------------------------------------------------------------
    // Display the introduction screen.
    // --------------------------------------------------------------------------------------------

    std::cout << "Cryptocoin auto-trading robot" << std::endl;
    std::cout << "Version 0.2b Copyright (c) Chris Morrison 2019" << std::endl << std::endl;

    // --------------------------------------------------------------------------------------------
    // Initialise the messaging system.
    // --------------------------------------------------------------------------------------------

    std::cout << utilities::timestamp() << " Initialising messaging system... ";

    try
    {
        message_dispatcher.initialise("o.VtG4grSgRPUjQnfWp0gPivCpkdDH3not");
    }
    catch (std::exception& ex)
    {
        std::cout << "FAILED: " << ex.what() << std::endl;
        return 1;
    }

    // message_dispatcher.set_target(communications::messaging::message_target::device_nickname, "Google Pixel 3 XL");
    // message_dispatcher.send_file("/home/chris/Downloads/Image.jpeg", "Incoming File", "This is a JPG file.");
    // message_dispatcher.send_file("/home/chris/Downloads/Image2.jpg", "", "");
    // message_dispatcher.send_file("/home/chris/Downloads/Image3.jpg", "", "This is a JPG file.");
    // message_dispatcher.send_file("/home/chris/Downloads/Image4.jpg", "Incoming File", "");
    // message_dispatcher.send_message("Title", "Message with Title");
    // message_dispatcher.send_message("", "Message without Title");
    // message_dispatcher.send_link("Title", "Link with Title", "https://www.youtube.com/watch?v=xe68tRovPss");
    // message_dispatcher.send_link("", "Link without Title", "https://www.youtube.com/watch?v=xe68tRovPss");
    // message_dispatcher.send_link("", "", "https://www.youtube.com/watch?v=xe68tRovPss");
    // message_dispatcher.relay_sms("+447542650289", "This is an automated text message.");

    std::cout << "DONE" << std::endl;

    std::cout << utilities::timestamp() << " Opening work order file...       ";

    // Verify that the work order file exists and is accessible.
    work_order_file.open(work_order_path, std::ios::in | std::ios::out);
    if (!work_order_file)
    {
        std::cout << "FAILED: " << strerror(errno) << std::endl;
        mtx.lock();
        std::cout << utilities::timestamp() << " Fatal error: failed to open the work order file: " << strerror(errno) << std::endl;
        mtx.unlock();
        return 1;
    }
    else
    {
        std::cout << "DONE" << std::endl;
    }

    // Read out the contents of the work order file.
    work_order_file >> next_op;
    work_order_file.clear(); // Make sure we are not EOF.
    work_order_file.seekg(0, std::ios::beg);

    if (next_op.empty())
    {
        mtx.lock();
        std::cout << utilities::timestamp() << " Fatal error: no instructions could be found in the work order file." << std::endl;
        mtx.unlock();

        return 1;
    }

    // Get the information from the instruction in the file.

    boost::split(strs, next_op, boost::is_any_of(":"));
    if (strs.size() != 5)
    {
        mtx.lock();
        std::cout << utilities::timestamp() << " Fatal error: the work order file did not contain the expected information and may be corrupted or damaged." << std::endl;
        mtx.unlock();

        return 1;
    }

    // The first parameter is the coin.
    coin = strs[0];

    // The second parameter is always the action we are required to take and will be on of the following
    // BUY
    // SELL
    // WFB
    // WFS
    action = strs[1];

    // The third parameter is the fiat currency to use.
    fiat = strs[2];

    // Create our trading context.
    std::stringstream init_string;
    init_string << "ee3f2635939845af5e1db506109aeeb6" << ":";
    init_string << "v3ty5dro4zq" << ":";
    init_string << "pSVf+fsikQrnc5UxlKxCQ15zBj68+UFoZE4v/9LFHiBiGsfLrDApu2YQyseAkl+IXhba/ihCmNrhqpM/Zdi3NQ==";

    cryptocoin::trading::coinbase_trade_context coinbase(init_string.str(), coin, fiat, print_change_update);

    mtx.lock();
    std::cout << utilities::timestamp() << " Using " << std::fixed << std::setprecision(2) << (fiat_percent * 100.00) << "% of the " << fiat << " fiat balance." << std::endl;
    mtx.unlock();

    while (execute_trade(coinbase)) {}

    return 1;
}

///
/// \param up
/// \param down
void print_change_update(long double up, long double down)
{
    mtx.lock();
    std::cout << utilities::timestamp() << " Note: updated sell increase rate to: " << (up * 100.00) << "%" << std::endl;
    std::cout << utilities::timestamp() << " Note: updated buy decrease rate to: " << (down * 100.00) << "%" << std::endl;
    mtx.unlock();
}

bool execute_trade(cryptocoin::trading::trade_context& context)
{
    long double old_price = 0.0;
    long double tenpc = 0.0;
    uint64_t new_price = 0;
    std::vector<std::string> strs;
    std::string next_op;

    work_order_file >> next_op;
    work_order_file.clear(); // Make sure we are not EOF.
    work_order_file.seekg(0, std::ios::beg);

    if (next_op.empty())
    {
        mtx.lock();
        std::cout << utilities::timestamp() << " Fatal error: no instructions could be found in the work order file." << std::endl;
        mtx.unlock();

        return false;
    }

    // Get the information from the instruction in the file.
    boost::split(strs, next_op, boost::is_any_of(":"));
    if (strs.size() != 5)
    {
        mtx.lock();
        std::cout << utilities::timestamp() << " Fatal error: the work order file did not contain the expected information and may be corrupted or damaged." << std::endl;
        mtx.unlock();

        return false;
    }

    // Verify that the coin still matches.
    if (strs[0] != coin)
    {
        std::cout << utilities::timestamp() << " Fatal error: unexpected information in the work order file." << std::endl;
        return false;
    }

    // The second parameter is always the action we are required to take and will be on of the following
    // BUY
    // SELL
    // WFB
    // WFS
    action = strs[1];

    // Verify that the fiat still matches.
    if (strs[2] != fiat)
    {
        std::cout << utilities::timestamp() << " Fatal error: unexpected information in the work order file." << std::endl;
        return false;
    }

    // ============================================================================================
    // We need to buy some coin at the price in the work order file.
    // ============================================================================================
    if (action == "BUY")
    {
        std::string buy_price(strs[3]);
        std::string current_price = context.current_price();

        // Make sure the price is right.
        long double bp = std::stold(buy_price);
        long double cp = std::stold(current_price);

        // If the current price has dropped below our buy price then update our buy price.
        if (cp < bp)
        {
            buy_price = current_price;
            bp = cp;
            mtx.lock();
            std::cout << utilities::timestamp() << " Updated buy price to current, better price of " << current_price << " " << fiat << std::endl;
            mtx.unlock();
        }

        // Work out order size.
        std::string sbal = context.fiat_balance();
        if (sbal.empty())
        {
            mtx.lock();
            std::cout << utilities::timestamp() << " Warning: failed to retrieve balance from server - retrying in 30 seconds." << std::endl;
            mtx.unlock();
            boost::this_thread::sleep(boost::posix_time::seconds(30));
            return true;
        }
        long double bal = std::stold(sbal);
        if (bal < 5.00)
        {
            mtx.lock();
            std::cout << utilities::timestamp() << " Fiat fiat_balance is less than 5.00 " << fiat << " - trading impossible." << std::endl;
            mtx.unlock();
            return false;
        }
        // Get the percentage of the fiat fiat_balance that we are allowed to use/
        bal *= fiat_percent;
        long double to_buy = bal / bp;
        std::string size = utilities::to_string_with_precision(to_buy, 2);
        char c;
        if ((c = size.back()) > '0')
        {
            size.pop_back();
            size.push_back(c - 1);
        }

        // Perform the trade.
        mtx.lock();
        std::cout << utilities::timestamp() << " Performing buy of " << size << " " << coin << " at " << buy_price << " " << fiat << " per coin with " << sbal << " " << fiat << "." << std::endl;
        mtx.unlock();
        std::string out_uuid;
        std::string str_new_price;
        cryptocoin::trading::order_status result = context.post_order(cryptocoin::trading::buy, cryptocoin::trading::limit, size, buy_price, "", out_uuid);
        switch (result)
        {
            case cryptocoin::trading::in_progress:
                work_order_file.seekp(0, std::ios::beg);
                work_order_file << coin << ":" << "WFB:" << fiat << ":" << buy_price << ":" << out_uuid << std::endl;
                work_order_file << std::string(40, ' ');
                work_order_file.flush();
                mtx.lock();
                std::cout << utilities::timestamp() << " Buy order posted - checking outcome in 10 minutes." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(10));
                return true;
            case cryptocoin::trading::completed:
                play_sound("/usr/share/auto-trader-bots/chaching1.wav");
                old_price = std::stold(buy_price);
                tenpc = old_price * context.sell_price_adjustment();
                if (tenpc < 1.00) tenpc = 1.00;
                new_price = old_price + tenpc;
                str_new_price = utilities::to_string_with_precision(new_price, 2);
                work_order_file.seekp(0, std::ios::beg);
                work_order_file << coin << ":" << "SELL:" << fiat << ":" << std::to_string(new_price) << ":NONE" << std::endl;
                work_order_file << std::string(40, ' ');
                work_order_file.flush();
                mtx.lock();
                std::cout << utilities::timestamp() << " The current buy order has completed successfully." << std::endl;
                mtx.unlock();
                return true;
            case cryptocoin::trading::network_error:
                mtx.lock();
                std::cout << utilities::timestamp() << " Warning: a temporary network error occurred - retrying in 1 minute." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(1));
                return true;
            case cryptocoin::trading::fatal_error:
                mtx.lock();
                std::cout << utilities::timestamp() << " A fatal error occurred - see the log file for details." << std::endl;
                mtx.unlock();
                return false;
            case cryptocoin::trading::insufficient_funds:
                mtx.lock();
                std::cout << utilities::timestamp() << " Warning: failed to post buy order due to insufficient fiat fiat_balance - retrying in 30 minutes." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(30));
                return true;
            default:
                mtx.lock();
                std::cout << utilities::timestamp() << " Warning: failed to post buy order - retrying in 30 seconds." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::seconds(30));
                return true;
        }
    }

    // ============================================================================================
    // A buy has been set up, we need to see if it has completed.
    // ============================================================================================
    if (action == "WFB")
    {
        std::string buy_price(strs[3]);
        std::string uuid = strs[4];
        std::string str_new_price;

        cryptocoin::trading::order_status result = context.get_order_status(uuid);
        switch (result)
        {
            case cryptocoin::trading::in_progress:
                mtx.lock();
                std::cout << utilities::timestamp() << " The current buy order has not yet completed - checking again in 10 minutes." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(10));
                return true;
            case cryptocoin::trading::completed:
                play_sound("/usr/share/auto-trader-bots/chaching1.wav");
                old_price = std::stold(buy_price);
                tenpc = old_price * context.sell_price_adjustment();
                if (tenpc < 1.00) tenpc = 1.00;
                new_price = old_price + tenpc;
                str_new_price = utilities::to_string_with_precision(new_price, 2);
                work_order_file.seekp(0, std::ios::beg);
                work_order_file << coin << ":" << "SELL:" << fiat << ":" << str_new_price << ":NONE" << std::endl;
                work_order_file << std::string(40, ' ');
                work_order_file.flush();
                mtx.lock();
                std::cout << utilities::timestamp() << " The current buy order has completed successfully." << std::endl;
                mtx.unlock();
                return true;
            case cryptocoin::trading::network_error:
                mtx.lock();
                std::cout << utilities::timestamp() << " Warning: a temporary network error occurred - retrying in 1 minute." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(1));
                return true;
            case cryptocoin::trading::cancelled:
                work_order_file.seekp(0, std::ios::beg);
                work_order_file << coin << ":" << "BUY:" << fiat << ":" << buy_price << ":NONE" << std::endl;
                work_order_file << std::string(40, ' ');
                work_order_file.flush();
                mtx.lock();
                std::cout << utilities::timestamp() << " The current buy order appears to have been cancelled - setting up for repost in 1 minute." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(1));
                return true;
            case cryptocoin::trading::fatal_error:
                mtx.lock();
                std::cout << utilities::timestamp() << " A fatal error occurred - see the log file for details." << std::endl;
                mtx.unlock();
                return false;
        }
    }

    // ============================================================================================
    // We need to buy some coin at the price in the work order file.
    // ============================================================================================
    if (action == "SELL")
    {
        std::string sell_price(strs[3]);
        std::string current_price = context.current_price();

        // Make sure the price is right.
        long double bp = std::stold(sell_price);
        long double cp = std::stold(current_price);

        // If the current price has risen above our buy price then update our buy price.
        if (cp > bp)
        {
            sell_price = current_price;
            bp = cp;
            std::cout << utilities::timestamp() << " Updated buy price to current, better price of " << current_price << " " << fiat << std::endl;
        }

        std::string sbal = context.coin_balance();
        if (sbal.empty())
        {
            mtx.lock();
            std::cout << utilities::timestamp() << " Warning: failed to retrieve fiat balance from server - retrying in 30 seconds." << std::endl;
            mtx.unlock();
            boost::this_thread::sleep(boost::posix_time::seconds(30));
            return true;
        }

        long double bal = std::stold(sbal);
        if (bal == 0.00)
        {

        }

        // Perform the trade.
        mtx.lock();
        std::cout << utilities::timestamp() << " Performing sell of " << sbal << " " << coin << " at " << sell_price << " " << fiat << " per coin ." << std::endl;
        mtx.unlock();
        std::string out_uuid;
        std::string str_new_price;
        cryptocoin::trading::order_status result = context.post_order(cryptocoin::trading::sell, cryptocoin::trading::limit, sbal, sell_price, "", out_uuid);
        switch (result)
        {
            case cryptocoin::trading::in_progress:
                work_order_file.seekp(0, std::ios::beg);
                work_order_file << coin << ":" << "WFS:" << fiat << ":" << sell_price << ":" << out_uuid << std::endl;
                work_order_file << std::string(40, ' ');
                work_order_file.flush();
                mtx.lock();
                std::cout << utilities::timestamp() << " Sell order posted - checking outcome in 10 minutes." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(10));
                return true;
            case cryptocoin::trading::completed:
                play_sound("/usr/share/auto-trader-bots/chaching2.wav");
                old_price = std::stold(sell_price);
                tenpc = old_price * context.buy_price_ajustment();
                if (tenpc < 1.00) tenpc = 1.00;
                new_price = old_price - tenpc;
                str_new_price = utilities::to_string_with_precision(new_price, 2);
                work_order_file.seekp(0, std::ios::beg);
                work_order_file << coin << ":" << "BUY:" << fiat << ":" << std::to_string(new_price) << ":NONE" << std::endl;
                work_order_file << std::string(40, ' ');
                work_order_file.flush();
                mtx.lock();
                std::cout << utilities::timestamp() << " The current sell order has completed successfully." << std::endl;
                mtx.unlock();
                return true;
            case cryptocoin::trading::network_error:
                mtx.lock();
                std::cout << utilities::timestamp() << " Warning: a temporary network error occurred - retrying in 1 minute." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(1));
                return true;
            case cryptocoin::trading::fatal_error:
                mtx.lock();
                std::cout << utilities::timestamp() << " A fatal error occurred - see the log file for details." << std::endl;
                mtx.unlock();
                return false;
            case cryptocoin::trading::insufficient_funds:
                mtx.lock();
                std::cout << utilities::timestamp() << " Warning: failed to post sell order due to insufficient " << coin << " fiat_balance - retrying in 30 minutes." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(30));
                return true;
            default:
                mtx.lock();
                std::cout << utilities::timestamp() << " Warning: failed to post sell order - retrying in 30 seconds." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::seconds(30));
                return true;
        }
    }

    // ============================================================================================
    // A sell order has been set up, we need to see if it has completed.
    // ============================================================================================
    if (action == "WFS")
    {
        std::string sell_price(strs[3]);
        std::string uuid = strs[4];
        std::string str_new_price;

        cryptocoin::trading::order_status result = context.get_order_status(uuid);
        switch (result)
        {
            case cryptocoin::trading::in_progress:
                mtx.lock();
                std::cout << utilities::timestamp() << " The current sell order has not yet completed - checking again in 10 minutes." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(10));
                return true;
            case cryptocoin::trading::completed:
                play_sound("/usr/share/auto-trader-bots/chaching2.wav");
                old_price = std::stold(sell_price);
                tenpc = old_price * context.buy_price_ajustment();
                if (tenpc < 1.00) tenpc = 1.00;
                new_price = old_price + tenpc;
                str_new_price = utilities::to_string_with_precision(new_price, 2);
                work_order_file.seekp(0, std::ios::beg);
                work_order_file << coin << ":" << "BUY:" << fiat << ":" << str_new_price << ":NONE" << std::endl;
                work_order_file << std::string(40, ' ');
                work_order_file.flush();
                mtx.lock();
                std::cout << utilities::timestamp() << " The current sell order has completed successfully." << std::endl;
                mtx.unlock();
                return true;
            case cryptocoin::trading::network_error:
                mtx.lock();
                std::cout << utilities::timestamp() << " Warning: a temporary network error occurred - retrying in 1 minute." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(1));
                return true;
            case cryptocoin::trading::cancelled:
                work_order_file.seekp(0, std::ios::beg);
                work_order_file << coin << ":" << "SELL:" << fiat << ":" << sell_price << ":NONE" << std::endl;
                work_order_file << std::string(40, ' ');
                work_order_file.flush();
                mtx.lock();
                std::cout << utilities::timestamp() << " The current sell order appears to have been cancelled - setting up for repost in 1 minute." << std::endl;
                mtx.unlock();
                boost::this_thread::sleep(boost::posix_time::minutes(1));
                return true;
            case cryptocoin::trading::fatal_error:
                mtx.lock();
                std::cout << utilities::timestamp() << " A fatal error occurred - see the log file for details." << std::endl;
                mtx.unlock();
                return false;
        }
    }

    work_order_file.close();
    return false;
}

void play_sound(const std::string& sound_file)
{

    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;
    int dir, pcmrc;

    short* buf = nullptr;
    int readcount;

    SF_INFO sfinfo;
    SNDFILE *infile = nullptr;

    infile = sf_open(sound_file.c_str(), SFM_READ, &sfinfo);

    /* Open the PCM device in playback mode */
    if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) != 0)
    {
        std::cout << "Failed to open the PCM device." << std::endl;
    }

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    /* Set parameters */
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, sfinfo.channels);
    snd_pcm_hw_params_set_rate(pcm_handle, params, sfinfo.samplerate, 0);

    /* Write parameters */
    snd_pcm_hw_params(pcm_handle, params);

    /* Allocate buffer to hold single period */
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);

    buf = static_cast<short *>(std::malloc(frames * sfinfo.channels * sizeof(short)));
    while ((readcount = sf_readf_short(infile, buf, frames)) > 0)
    {

        pcmrc = snd_pcm_writei(pcm_handle, buf, readcount);
        if (pcmrc == -EPIPE)
        {
            snd_pcm_prepare(pcm_handle);
        }
        else if (pcmrc < 0)
        {
            std::cout << "Error writing to PCM device: " << snd_strerror(pcmrc) << std::endl;
        }
        else if (pcmrc != readcount)
        {
            std::cout << "PCM write differs from PCM read." << std::endl;
        }

    }

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    std::free(buf);
}





