#include "alloc.h"
#include "fcntl.h"

/**가상 메모리 주소 공간에 파일을 매핑하여 메모리 주소에 직접 접근**/
typedef struct{
	int size;
	char* addr; 
	int used; 
}_chunkInfo;

char *src; 
char *addr; 
int fd, ARR_SIZE; 
_chunkInfo chunk[PAGESIZE/MINALLOC]; 

int getUsedSize();
void Bubble_Sort();
int init_alloc()
{
	src = NULL;
	int i=0; 
	ARR_SIZE = PAGESIZE/MINALLOC; //최대 배열크기
	
	if(!access("./chunk_file",0)) //이미 mapping할 file이 존재시 삭제하고 새로 만듦
		rmdir("./chunk_file");

	if((fd=open("chunk_file", O_RDWR|O_CREAT))==-1)
	{
		fprintf(stderr, "fd open error\n");
		return 1; 
	}
	
	if(ftruncate(fd, PAGESIZE) == -1){ //mapping할 file 을 4kb로 만듦
		fprintf(stderr, "fd error\n");
		return 1; 
	}

	if((src = (char*)mmap(0, PAGESIZE,PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))==(void*)-1)
	{
		perror("mmap error\n");
		return 1; 
	}

	//초기화
	for(i=0; i<ARR_SIZE; i++)
	{
		chunk[i].size = -1; 
		chunk[i].addr = NULL; 
		chunk[i].used = -1; 
	}

	//printf(" int init_alloc()\n");
	return 0; 
}

int cleanup()//
{
	//printf(" int cleanup()\n");

	 if(munmap(addr, PAGESIZE) == -1) 
	 { 
		fprintf(stderr, "cleanup err\n");
	 	return 1; 
	 }

	close(fd); 

	return 0;

}

char *alloc(int size)
{
	int i, j, temp, flag = 0; 
	char * getAddr;
	int totalSize = getUsedSize(); 

	//printf("alloc \n");
	if(size%MINALLOC || (PAGESIZE-totalSize < size)){//남은 사이즈보다 큰 사이즈를 매핑할때
		//한번 더 거르는 과정 필요! --> chunk단위로 비교도 해야함!!
		fprintf(stderr, "alloc error\n");
		return NULL; 
	}

	//근접한 청크를 붙임
	_chunkInfo chunk_temp; 
	int total=0; 
	flag = -1;
	for(i=0; i<ARR_SIZE; i++)
	{	
		//현재 청크로부터 다음 청크에 업데이트가 가능한 경우일 때의 현재 청크 
		if(flag == -1 && chunk[i].used == -2 && chunk[i+1].used == -2)
		{
			chunk_temp = chunk[i]; //처음 변수 인덱스
				temp = i;
			
			total += chunk[i].size; 	
			flag = 0; 
		}
		//현재 청크로부터 다음 청크에 업데이트가 가능한 경우일 때의 다음 청크 
		else if(flag == 0)
		{
			if(chunk[i].used == -2)//다음 청크도 업데이트 가능할 경우 청크 사이즈를 더함
				total += chunk[i].size; 	
			else{//업데이트 불가능할 경우 이전 청크들을 합칠 청크 중 첫번째 청크에 정보 대입
				//-1, 1일 때
				j = i-1; 
				//total을 조작
				while(chunk[j].addr > chunk_temp.addr)
				{
					chunk[j].used = -1;//안쓰는 청크 비활성화
					--j;
				}
				chunk[temp].size = total; 
				chunk[temp].used = -2; //업데이트 가능한 형태로 만듦
				break;
			}
		}

	}
	


	
	for(i=0; i<ARR_SIZE; i++)
	{
		if(chunk[i].used == -2) //if for update 
		{
			flag = 1; 
			if(chunk[i].size > size) //청크 사이즈가 더 클때
			{
				temp = chunk[i].size; //할당 전 청크 사이즈 
				chunk[i].used = 1; 
				chunk[i].size = size; 
				
				for(j=0; j<ARR_SIZE; j++) //할당하고 남은 사이즈를 가지고 파일의 끝에 할당 
				{
					if(chunk[j].used == -1)
					{
						chunk[j].used = -2; 
						chunk[j].size = temp - size; 
						chunk[j].addr = chunk[i].addr + size;
						Bubble_Sort(); 
						getAddr = chunk[i].addr; 
						break;
					}
				}
			}
			if(chunk[i].size == size)
			{
				chunk[i].used = 1; 
				chunk[i].size = size; 
				getAddr = chunk[i].addr; 
				break;
			}

		
		}
	}

	//처음 채워갈때 || update할 chunk를 모두 뒤져봐도 할당할 사이즈가 더 클 때
	//파일의 끝, 구조체 배열이 -1인 곳에 할당
	//error check. 남은사이즈가 더 작을 때 fprintf
	int ord = 0; 
	if(flag == 0 || i == ARR_SIZE)
		for(i=0; i<ARR_SIZE; i++)
		{
			ord++; 
			//if(chunk[i].used == -1 || chunk[i].used == -2 ) 
			if(chunk[i].used == -1) 
			{
				
				chunk[i].size = size; 
				chunk[i].used = 1; 
				if(i==0){
					chunk[i].addr = src;
					//printf("%d	%p\n", i, chunk[i].addr);
				}
				else {
					//printf("%d-> %d\n", i-1, chunk[i-1].size);
					chunk[i].addr = chunk[i-1].addr + chunk[i-1].size; 
					//printf("%d	%p\n", i, chunk[i].addr);
				}
				Bubble_Sort();
				getAddr = chunk[i].addr; 
				//printf("[ %p ]\n", getAddr); 
				break; 
			}
		}

	if(i==ARR_SIZE){ //size를 mapping할 chunk가 더이상 남아있지 않음
		fprintf(stderr, "no place for size %d alloc\n", size);
		return NULL; 
	}

	//check();
	
	///printf("=== 	getAddr : %p , chun.addr : %p\n", getAddr, chunk[i].addr);
	//printf("getAddr => %p\n\n", getAddr);
	return getAddr; 

	
}
	
void Bubble_Sort()
{
	_chunkInfo temp; 
	int i, j, len;

	len = 0;

	for(i=0; i<ARR_SIZE; i++)
	{
		if(chunk[i].used != -1){
			len++;
		}
	}

	//printf("len is %d\n", len);
	for(int i = len - 1; i > 0; i--)
		for (int j = 0; j < i; j++)
			if (chunk[j].addr > chunk[j+1].addr)
			{
				temp = chunk[j];
				chunk[j] = chunk[j + 1];
				chunk[j + 1] = temp;
			}


	//printf("\n");
}

int getUsedSize()
{
	int total, i;
	total = 0; 

	for(i=0; i<ARR_SIZE; i++){
		if(chunk[i].used == 1){
			total+=chunk[i].size;
		}
	}

	//printf("totally used size ==> %d\n", total);
	return total; 
	
}

void dealloc(char * buf)//cunk를 해체 
{
	int i;
	//printf(" 		void dealloc(char *) : %p\n", buf);
	
	for(i=0; i<ARR_SIZE; i++)
	{
		if(chunk[i].used == 1 && chunk[i].addr == buf)
		{
			//printf("%d.ADDR -> %p\n", i, buf); 
			chunk[i].used = -2; 
		}
	}
}



void check(){
	/*
	printf("\n");
	int i;
	for(i=0; i<ARR_SIZE; i++){
			
		if(chunk[i].used != -1) 
		{
		printf("%d.size	 : %d\n", i, chunk[i].size);
		printf("%d.used  : %d\n", i, chunk[i].used);
		printf("%d.addr  : %p\n", i, chunk[i].addr);
	
		}
	}
	printf("\n");
	*/

	printf("\n");
	int i;
	for(i=0; i<10; i++){
		printf("%d.size	 : %d\n", i, chunk[i].size);
		printf("%d.used  : %d\n", i, chunk[i].used);
		printf("%d.addr  : %p\n", i, chunk[i].addr);
	
	}
	printf("\n\n");


}


