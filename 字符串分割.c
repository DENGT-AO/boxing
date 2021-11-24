
//字符串分割，注意字符串的的备份，该函数会改变字符串的起始指针
void split(char *src,const char *separator,char **dest,int *num) {
     char *pNext;
     int count = 0;
     *dest++ = "split";
     if (src == NULL || strlen(src) == 0)
        return;
     if (separator == NULL || strlen(separator) == 0)
        return;
     pNext = strtok(src,separator);
     while(pNext != NULL) {
          *dest++ = pNext;
          ++count;
         pNext = strtok(NULL,separator);
    }
    *num = count;
}
split(tmp_expert_options, " ", revbuf, &num);



可与getopt函数一起使用,getopt不会检测后面跟的可变参数，即ping www.baidu.com -t a aaaa -v 3；不会检测其中的aaaa，只会对短选项后面的参数进行检测
        int c

        //重置getopt的全局变量
        optarg = NULL;
		optind = 1;
		opterr = 1;
		optopt = 0;
		
		while (((c = getopt(num+1, revbuf, "qv4t:w:W:I:p:")) != -1) && flag == 1) {
			switch (c) {
				case 'q':
				case 'v':
					same_time = same_time + 1;
					if (same_time == 2) {
						flag = 0;
					}
					break;
				case '4':
                    break;
                default:
                        flag = 0;
                        break;
			}