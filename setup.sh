sudo apt update

# Installing Vulkan
wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.3.268-jammy.list https://packages.lunarg.com/vulkan/1.3.268/lunarg-vulkan-1.3.268-jammy.list
sudo apt update
sudo apt install vulkan-sdk
echo "--------------------------------------------INSTALLED VULKAN"

# Installing xcb libraries
sudo apt install pkg-config
sudo apt install automake
sudo apt install autoconf
sudo apt install check
sudo apt install xsltproc
sudo apt install gperf
sudo apt install xcb
sudo apt install xorg-dev
sudo apt install libxkbcommon-dev
echo "--------------------------------------------INSTALLED XCB LIBRARIES"