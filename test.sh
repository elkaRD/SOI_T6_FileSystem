#!/bin/bash
echo "################################################################" >> result.txt
echo "Create disk:" > result.txt
./a.out new disk 40000000 >> result.txt
./a.out info disk >> result.txt

echo "################################################################" >> result.txt
echo "Insert few small files" >> result.txt
./a.out insert disk small.txt t1.txt >> result.txt
./a.out insert disk small.txt t2.txt >> result.txt
./a.out insert disk small.txt t3.txt >> result.txt
./a.out memory disk >> result.txt
./a.out list disk >> result.txt
./a.out info disk >> result.txt

echo "################################################################" >> result.txt
echo "Insert medium and small files" >> result.txt
./a.out insert disk logo.bmp pic1.bmp >> result.txt
./a.out insert disk small.txt t4.txt >> result.txt
./a.out insert disk doc.pdf doc1.pdf >> result.txt
./a.out insert disk small.txt t5.txt >> result.txt
./a.out insert disk logo.bmp pic2.bmp >> result.txt
./a.out memory disk >> result.txt
./a.out list disk >> result.txt
./a.out info disk >> result.txt

echo "################################################################" >> result.txt
echo "Remove few files" >> result.txt
./a.out delete disk t2.txt >> result.txt
./a.out delete disk pic1.bmp >> result.txt
./a.out delete disk doc1.pdf >> result.txt
./a.out memory disk >> result.txt
./a.out list disk >> result.txt
./a.out info disk >> result.txt

echo "################################################################" >> result.txt
echo "Insert large file" >> result.txt
./a.out insert disk large.png large1.png >> result.txt
./a.out memory disk >> result.txt
./a.out list disk >> result.txt
./a.out info disk >> result.txt

echo "################################################################" >> result.txt
echo "Export pic2.bmp and large.png" >> result.txt
./a.out export disk pic2.bmp exported_pic.bmp >> result.txt
./a.out export disk large1.png exported_pic.png >> result.txt

echo "################################################################" >> result.txt
echo "Remove everything except large file" >> result.txt
./a.out delete disk t1.txt >> result.txt
./a.out delete disk t3.txt >> result.txt
./a.out delete disk t4.txt >> result.txt
./a.out delete disk t5.txt >> result.txt
./a.out delete disk pic2.bmp >> result.txt
./a.out memory disk >> result.txt
./a.out list disk >> result.txt
./a.out info disk >> result.txt

echo "################################################################" >> result.txt
echo "Remove the last (large) file" >> result.txt
./a.out delete disk large1.png >> result.txt
./a.out memory disk >> result.txt
./a.out list disk >> result.txt
./a.out info disk >> result.txt

echo "################################################################" >> result.txt
echo "Remove the disk" >> result.txt
./a.out remove disk Y >> result.txt
