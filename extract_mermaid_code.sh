#!/bin/bash
# Extract C++ comment lines, which start with //--
sed -n 's/.*\/\/-- //p' $1

# Copy & paste the output to the live editor at
# https://mermaid-js.github.io/mermaid-live-editor
