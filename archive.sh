#!/bin/bash
git clone https://github.com/thenathattify/Marker.git marker
cd marker
git submodule update --init --recursive
rm -rf .git .gitignore .gitmodules
cd ..
zip -r marker.zip marker
rm -rf marker
