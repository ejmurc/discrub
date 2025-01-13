#!/bin/bash

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null
then
  echo "clang-format could not be found. Exiting..."
  exit 1
fi

# Format the files
for file in src/*.c include/*.h; do
  clang-format -i "$file"
  echo "Formatted: $file"
done

echo "Formatting complete."
