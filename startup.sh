git clone https://github.com/axboe/liburing.git /root/liburing
cd /root/liburing
./configure
make -j$(nproc)
make install
