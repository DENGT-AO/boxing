all:
	@echo ""
	@echo "Start compiling......"
	@echo ""
	gcc -o sub sub.c -lmosquitto
	gcc -o pub pub.c -lmosquitto
	gcc -o sub_2 sub_2.c -lmosquitto
	@echo "end"
sub:
	gcc -o sub sub.c -lmosquitto 
pub:
	gcc -o pub pub.c -lmosquitto 
sub_2:
	gcc -o sub_2 sub_2.c -lmosquitto
clean:
	rm sub pub
