#!/usr/bin/env bash

rm ./btns/*

buttonName="playButton"

./generate_hex_button.py --label "PLAY" --output "./btns/${buttonName}.png"

filePtxPath=./btns/${buttonName}.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

./generate_hex_button.py --label "PLAY" --pressed --output "./btns/${buttonName}Pressed.png"


filePtxPath=./btns/${buttonName}Pressed.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

buttonName="OnePlayerButton"

./generate_hex_button.py --label "1 PLAYER" --color yellow --output "./btns/${buttonName}.png"

filePtxPath=./btns/${buttonName}.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

./generate_hex_button.py --label "1 PLAYER" --color yellow --pressed  --output "./btns/${buttonName}Pressed.png"

filePtxPath=./btns/${buttonName}Pressed.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

buttonName="MultiplayerButton"

./generate_hex_button.py --label "MULTIPLAYER" --color blue --output "./btns/${buttonName}.png"

filePtxPath=./btns/${buttonName}.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

./generate_hex_button.py --label "MULTIPLAYER" --color blue --pressed --output "./btns/${buttonName}Pressed.png"

filePtxPath=./btns/${buttonName}Pressed.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

buttonName="BackButton"

./generate_hex_button.py --label "BACK" --color green --output "./btns/${buttonName}.png" --out_w 64 --out_h 32 --font_size 10

filePtxPath=./btns/${buttonName}.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

./generate_hex_button.py --label "BACK" --color green --pressed --output "./btns/${buttonName}Pressed.png" --out_w 64 --out_h 32 --font_size 10

filePtxPath=./btns/${buttonName}Pressed.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF


buttonName="PrevPlayerButton"

./generate_hex_button.py --label "<" --color_tuple "(235, 89, 40)" --output "./btns/${buttonName}.png" --out_w 32 --out_h 32 --font_size 10

filePtxPath=./btns/${buttonName}.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

./generate_hex_button.py --label "<" --color_tuple "(235, 89, 40)" --pressed --output "./btns/${buttonName}Pressed.png" --out_w 32 --out_h 32 --font_size 10

filePtxPath=./btns/${buttonName}Pressed.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

buttonName="NextPlayerButton"

./generate_hex_button.py --label ">" --color_tuple "(235, 89, 40)" --output "./btns/${buttonName}.png" --out_w 32 --out_h 32 --font_size 10

filePtxPath=./btns/${buttonName}.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

./generate_hex_button.py --label ">" --color_tuple "(235, 89, 40)" --pressed --output "./btns/${buttonName}Pressed.png" --out_w 32 --out_h 32 --font_size 10

filePtxPath=./btns/${buttonName}Pressed.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF


buttonName="EmptyPlayerNumberButton"

./generate_hex_button.py --label "" --color_tuple "(235, 89, 40)" --output "./btns/${buttonName}.png"

filePtxPath=./btns/${buttonName}.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF



buttonName="StartGameButton"

./generate_hex_button.py --label "START" --color_tuple "(155, 34, 230)" --output "./btns/${buttonName}.png" --out_w 64 --out_h 32 --font_size 10

filePtxPath=./btns/${buttonName}.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF

./generate_hex_button.py --label "START" --color_tuple "(155, 34, 230)" --pressed --output "./btns/${buttonName}Pressed.png" --out_w 64 --out_h 32 --font_size 10


filePtxPath=./btns/${buttonName}Pressed.ptxc

echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -cm 1024 -f tex4x4
EOF
