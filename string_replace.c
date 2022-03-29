//将str中的oldstr替换成newstr，替换成的字符串存放于outstr

int str_replcae(char *str,char *outstr,char *oldstr,char *newstr){
	char bstr[strlen(str)];
	memset(bstr,0,sizeof(bstr));
	int i=0;
	for(i = 0;i < strlen(str);i++){
	if(!strncmp(str+i,oldstr,strlen(oldstr))){
		strcat(bstr,newstr);
		i += strlen(oldstr) - 1;
	}else{
		strncat(bstr,str + i,1);
		}
	}
 
	strcpy(outstr,bstr);
	return 0;
}
