#!/bin/sh

GIT_VERS=`git rev-parse --short HEAD`
echo "Current GIT Version: $GIT_VERS"

FILE="version.vhd"

echo "Updating file: $FILE"
echo -e "library IEEE;\nuse IEEE.STD_LOGIC_1164.all;\nuse IEEE.NUMERIC_STD.all;\n\npackage version_pkg is\n    constant c_FW_VERSION : std_logic_vector(31 downto 0) := x\"0$GIT_VERS\";\nend version_pkg;" > $FILE

echo "Done!"
