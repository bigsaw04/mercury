echo "Running aclocal..."
aclocal --force
echo "Running autoheader..."
autoheader --force
echo "Running automake..."
automake --add-missing --copy --force-missing
echo "Running autoconf..."
autoconf --force
