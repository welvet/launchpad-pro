[![Build Status](https://travis-ci.org/dvhdr/launchpad-pro.svg?branch=master)](https://travis-ci.org/dvhdr/launchpad-pro)

# Launchpad Pro Sequencer
## Build

On macOS you can easily install the GCC ARM toolchain using the [homebrew package manager](http://brew.sh). The EABI tools are maintained in an external repository which you need to put on tap first. You can then run ```make``` to directly compile the code:

```
brew tap PX4/homebrew-px4
brew install gcc-arm-none-eabi
make
```

