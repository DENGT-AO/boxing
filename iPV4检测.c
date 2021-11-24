

//输入字符串，返回true或者false，注意IP地址备份，该函数ip地址起始指针
bool IsIpv4(char*str)
{
    char* ptr;
    int count = 0;
    const char *p = str;

    //1、判断是不是三个 ‘.’
    //2、判断是不是先导0
    //3、判断是不是四部分数
    //4、第一个数不能为0

    while(*p !='\0')
    {
        if(*p == '.')
        count++;
        p++;
    }

    if(count != 3)  {
		return false;
	}
    count = 0;
    ptr = strtok(str,".");
    while(ptr != NULL)
    {   
        count++;
        if(ptr[0] == '0' && isdigit(ptr[1])) {
			return false;
		}
        int a = atoi(ptr);
        if(count == 1 && a == 0) {
			return false;
		} 
        if(a<0 || a>255) {
			return false;
		}
        ptr = strtok(NULL, ".");

    }
    if(count == 4)  {
		return true;
	} else{
		return false;
	}  
}