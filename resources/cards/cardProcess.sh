#!/usr/bin/env bash

rm ./png/*
./generate_card.py

filePtxPath=./png/card_0.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_1.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_2.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_3.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_4.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_5.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_6.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_7.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_8.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_9.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_10.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_11.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_12.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_back.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_n1.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF

filePtxPath=./png/card_n2.ptxc
echo >> $filePtxPath

cat > $filePtxPath <<EOF
-gt -og -v -f palette16
EOF
