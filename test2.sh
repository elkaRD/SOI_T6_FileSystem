#!/bin/bash

echo -e "\n\tinfo error" > result2.txt
./a.out info error >> result2.txt
echo -e "\n\tnew disk 1000000" >> result2.txt
./a.out new disk 1000000 >> result2.txt

echo -e "\n\texport disk error.txt" >> result2.txt
./a.out export disk error.txt >> result2.txt
echo -e "\n\tinsert disk small.txt" >> result2.txt
./a.out insert disk small.txt >> result2.txt
echo -e "\n\tinsert disk small.txt" >> result2.txt
./a.out insert disk small.txt >> result2.txt
echo -e "\n\tinsert disk eororororor.txt" >> result2.txt
./a.out insert disk eororororor.txt new.txt >> result2.txt
echo -e "\n\tdelete disk fdsf" >> result2.txt
./a.out delete disk fdsf >> result2.txt
echo -e "\n\tdelete disk small.txt" >> result2.txt
./a.out delete disk small.txt >> result2.txt
echo -e "\n\texport disk small.txt exported.txt" >> result2.txt
./a.out export disk small.txt exported.txt >> result2.txt
echo -e "\n\tdelete disk small.txt" >> result2.txt
./a.out delete disk small.txt >> result2.txt

echo -e "\n\tinsert disk doc.pdf doc1.pdf" >> result2.txt
./a.out insert disk doc.pdf doc1.pdf >> result2.txt
echo -e "\n\tinsert disk doc.pdf doc1.pdf" >> result2.txt
./a.out insert disk doc.pdf doc2.pdf >> result2.txt
echo -e "\n\tinsert disk doc.pdf doc1.pdf" >> result2.txt
./a.out insert disk doc.pdf doc3.pdf >> result2.txt

./a.out remove disk Y >> result2.txt;
