#include "MingOS.h"
using namespace std;

//����ʵ��
void Ready()	//��¼ϵͳǰ��׼������,������ʼ��+ע��+��װ
{
	//��ʼ������
	nextUID = 0;
	nextGID = 0;
	isLogin = false;
	strcpy(Cur_User_Name,"root");//��ǰ�û���
	strcpy(Cur_Group_Name,"root");//��ǰ�û���

	//��ȡ������
	memset(Cur_Host_Name,0,sizeof(Cur_Host_Name));  //��ȡ������
	DWORD k= 100;  
	GetComputerName(Cur_Host_Name,&k);

	//��Ŀ¼inode��ַ ����ǰĿ¼��ַ������
	Root_Dir_Addr = Inode_StartAddr;	//��һ��inode��ַ
	Cur_Dir_Addr = Root_Dir_Addr;//��ǰ��Ŀ¼��ַRoot
	strcpy(Cur_Dir_Name,"/");//��ǰĿ¼����


	char c;
	printf("�Ƿ��ʽ��?[y/n]");
	while(c = getch()){
		fflush(stdin);
		if(c=='y'){
			printf("\n");
			printf("�ļ�ϵͳ���ڸ�ʽ������\n");
			if(!Format()){//���ļ�ϵͳ��ʽ��
				printf("�ļ�ϵͳ��ʽ��ʧ��\n");
				return ;
			}
			printf("��ʽ�����\n");
			break;
		}
		else if(c=='n'){//����ʽ��
			printf("\n");
			break;
		}
	}

	//printf("�����ļ�ϵͳ����\n");
	if(!Install()){			//��װ�ļ�ϵͳ
		printf("��װ�ļ�ϵͳʧ��\n");
		return ;
	}
	//printf("�������\n");
}

bool Format()	//��ʽ��һ����������ļ�
{
	int i,j;

	//��ʼ��������
	superblock->s_INODE_NUM = INODE_NUM;//inode����
	superblock->s_BLOCK_NUM = BLOCK_NUM;//������
	superblock->s_SUPERBLOCK_SIZE = sizeof(SuperBlock);//�������С
	superblock->s_INODE_SIZE = INODE_SIZE;//inode��С
	superblock->s_BLOCK_SIZE = BLOCK_SIZE;//���С
	superblock->s_free_INODE_NUM = INODE_NUM;//����inode����
	superblock->s_free_BLOCK_NUM = BLOCK_NUM;//���п�����
	superblock->s_blocks_per_group = BLOCKS_PER_GROUP;//ÿ�����
	superblock->s_free_addr = Block_StartAddr;	//���п��ջָ��Ϊ��һ��block
	superblock->s_Superblock_StartAddr = Superblock_StartAddr;//�������ַ
	superblock->s_BlockBitmap_StartAddr = BlockBitmap_StartAddr;//��λͼ��ַ
	superblock->s_InodeBitmap_StartAddr = InodeBitmap_StartAddr;//inodeλͼ��ַ
	superblock->s_Block_StartAddr = Block_StartAddr;//���ַ
	superblock->s_Inode_StartAddr = Inode_StartAddr;//inode��ַ
	//���п��ջ�ں��渳ֵ

	//��ʼ��inodeλͼ
	memset(inode_bitmap,0,sizeof(inode_bitmap));//inodeλͼ��ֵȫ������Ϊ0
	fseek(fw,InodeBitmap_StartAddr,SEEK_SET);//Lȷ��λ�ÿ�ʼд��
	fwrite(inode_bitmap,sizeof(inode_bitmap),1,fw);//Lд�뺯��

	//��ʼ��blockλͼ
	memset(block_bitmap,0,sizeof(block_bitmap));//blockλͼ��ֵȫ������Ϊ0
	fseek(fw,BlockBitmap_StartAddr,SEEK_SET);//Lȷ��λ�ÿ�ʼд��
	fwrite(block_bitmap,sizeof(block_bitmap),1,fw);//Lд�뺯��
	
	//��ʼ�����̿��������ݳ������ӷ���֯	
	for(i=BLOCK_NUM/BLOCKS_PER_GROUP-1;i>=0;i--){	//һ��INODE_NUM/BLOCKS_PER_GROUP�飬һ��FREESTACKNUM��128�������̿� ����һ�����̿���Ϊ����
		if(i==BLOCK_NUM/BLOCKS_PER_GROUP-1)
			superblock->s_free[0] = -1;	//û����һ�����п���
		else
			superblock->s_free[0] = Block_StartAddr + (i+1)*BLOCKS_PER_GROUP*BLOCK_SIZE;	//ָ����һ�����п�
		for(j=1;j<BLOCKS_PER_GROUP;j++){
			superblock->s_free[j] = Block_StartAddr + (i*BLOCKS_PER_GROUP + j)*BLOCK_SIZE;//ָ����һ�����п�
		}
		fseek(fw,Block_StartAddr+i*BLOCKS_PER_GROUP*BLOCK_SIZE,SEEK_SET);//Lȷ��λ�ÿ�ʼд��
		fwrite(superblock->s_free,sizeof(superblock->s_free),1,fw);	//����������̿飬512�ֽ�
	}
	//������д�뵽��������ļ�
	fseek(fw,Superblock_StartAddr,SEEK_SET);//Lȷ��λ�ÿ�ʼд��
	fwrite(superblock,sizeof(SuperBlock),1,fw);//Lд�뺯��

	fflush(fw);//ˢ�»�����

	//��ȡinodeλͼ
	fseek(fr,InodeBitmap_StartAddr,SEEK_SET);//Lȷ��λ�ÿ�ʼ��ȡ
	fread(inode_bitmap,sizeof(inode_bitmap),1,fr);//L��ȡ����

	//��ȡblockλͼ
	fseek(fr,BlockBitmap_StartAddr,SEEK_SET);//	Lȷ��λ�ÿ�ʼ��ȡ
	fread(block_bitmap,sizeof(block_bitmap),1,fr);//L��ȡ����

	fflush(fr);//ˢ�»�����

	//������Ŀ¼ "/"
		Inode cur;

		//����inode
		int inoAddr = ialloc(); //L����i�ڵ�������������inode��ַ

		//�����inode������̿�
		int blockAddr = balloc();//L���̿���亯��

		//��������̿������һ����Ŀ "."
		DirItem dirlist[16] = {0};//L��ʼ��һ����Ŀ¼
		strcpy(dirlist[0].itemName,".");
		dirlist[0].inodeAddr = inoAddr;//L����inode��ַ

		//д�ش��̿�
		fseek(fw,blockAddr,SEEK_SET);	//Lȷ��λ�ÿ�ʼд��
		fwrite(dirlist,sizeof(dirlist),1,fw);//Lд�뺯��

		//��inode��ֵ
		cur.i_ino = 0;//L����inode��
		cur.i_atime = time(NULL);//L�����ļ���������ʱ��
		cur.i_ctime = time(NULL);//L����inode����޸�ʱ��
		cur.i_mtime = time(NULL);//L�����ļ���������޸�ʱ��
		strcpy(cur.i_uname,Cur_User_Name);//L�����û���
		strcpy(cur.i_gname,Cur_Group_Name);//L�����û�����
		cur.i_cnt = 1;	//һ�����ǰĿ¼,"."
		cur.i_dirBlock[0] = blockAddr;//L���õ�һ��Ŀ¼���ַ
		for(i=1;i<10;i++){
			cur.i_dirBlock[i] = -1;
		}
		cur.i_size = superblock->s_BLOCK_SIZE;//L�����ļ���С
		cur.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
		cur.i_mode = MODE_DIR | DIR_DEF_PERMISSION;//L�����ļ�Ȩ��


		//д��inode
		fseek(fw,inoAddr,SEEK_SET);	//Lȷ��λ�ÿ�ʼд��
		fwrite(&cur,sizeof(Inode),1,fw);//Lд�뺯��
		fflush(fw);//ˢ�»�����

	//����Ŀ¼�������ļ�
	mkdir(Root_Dir_Addr,"home");	//�û�Ŀ¼//����Ŀ¼
	cd(Root_Dir_Addr,"home");//���뵱ǰĿ¼�µ�nameĿ¼
	mkdir(Cur_Dir_Addr,"root");		//LĿ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼��
	//�û���������˻� �Լ�����Ա��һ���˻����붼�洢��/etc/passwd�ļ��� ���ڵ�¼ʱ�Ĵ�ȡ�Լ���������Ĺ���鿴�ȵ�	
	cd(Cur_Dir_Addr,"..");//������һ��Ŀ¼
	mkdir(Cur_Dir_Addr,"etc");	//�����ļ�Ŀ¼
	cd(Cur_Dir_Addr,"etc");//���뵱ǰĿ¼�µ�nameĿ¼

	char buf[1000] = {0};//L��ʼ��һ���ջ�����
	
	sprintf(buf,"root:x:%d:%d\n",nextUID++,nextGID++);	//������Ŀ���û������������룺�û�ID���û���ID
	create(Cur_Dir_Addr,"passwd",buf);	//�����û���Ϣ�ļ�

	sprintf(buf,"root:root\n");	//������Ŀ���û���������
	create(Cur_Dir_Addr,"shadow",buf);	//�����û������ļ�
	chmod(Cur_Dir_Addr,"shadow",0660);	//�޸�Ȩ�ޣ���ֹ�����û���ȡ���ļ�

	sprintf(buf,"root::0:root\n");	//���ӹ���Ա�û��飬�û����������һ��Ϊ�գ�����û��ʹ�ã������ʶ�ţ������û��б��ã��ָ���
	sprintf(buf+strlen(buf),"user::1:\n");	//������ͨ�û��飬�����û��б�Ϊ��
	create(Cur_Dir_Addr,"group",buf);	//�����û�����Ϣ�ļ�

	cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼

	return true;
}

bool Install()	//��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
{
	//��д��������ļ�����ȡ�����飬��ȡinodeλͼ��blockλͼ����ȡ��Ŀ¼����ȡetcĿ¼����ȡ����ԱadminĿ¼����ȡ�û�xiaoĿ¼����ȡ�û�passwd�ļ���

	//��ȡ������
	fseek(fr,Superblock_StartAddr,SEEK_SET);//fseek�������������ļ�ָ���λ�ã�SEEK_SET��ʾ���ļ���ͷ��ʼ
	fread(superblock,sizeof(SuperBlock),1,fr);//fread�������Ǵ��ļ��ж�ȡ���ݣ���ȡһ�����ݿ�

	//��ȡinodeλͼ
	fseek(fr,InodeBitmap_StartAddr,SEEK_SET);//fseek�������������ļ�ָ���λ�ã�SEEK_SET��ʾ���ļ���ͷ��ʼ
	fread(inode_bitmap,sizeof(inode_bitmap),1,fr);//fread�������Ǵ��ļ��ж�ȡ���ݣ���ȡ�����ݴ���ڵ�һ������ָ����

	//��ȡblockλͼ
	fseek(fr,BlockBitmap_StartAddr,SEEK_SET);//fseek�������������ļ�ָ���λ�ã�SEEK_SET��ʾ���ļ���ͷ��ʼ
	fread(block_bitmap,sizeof(block_bitmap),1,fr);//fread�������Ǵ��ļ��ж�ȡ���ݣ���ȡ�����ݴ���ڵ�һ������ָ����

	return true;
}

void printSuperBlock()		//��ӡ��������Ϣ
{
	printf("\n");
	printf("����inode�� / ��inode�� ��%d / %d\n",superblock->s_free_INODE_NUM,superblock->s_INODE_NUM);//����inode�� / ��inode��
	printf("����block�� / ��block�� ��%d / %d\n",superblock->s_free_BLOCK_NUM,superblock->s_BLOCK_NUM);//����block�� / ��block��
	printf("��ϵͳ block��С��%d �ֽڣ�ÿ��inodeռ %d �ֽڣ���ʵ��С��%d �ֽڣ�\n",superblock->s_BLOCK_SIZE,superblock->s_INODE_SIZE,sizeof(Inode));//��ϵͳ block��С��block��С��ÿ��inodeռ inode��С����ʵ��С��inode��С��
	printf("\tÿ���̿��飨���ж�ջ��������block������%d\n",superblock->s_blocks_per_group);//ÿ���̿��飨���ж�ջ��������block����
	printf("\t������ռ %d �ֽڣ���ʵ��С��%d �ֽڣ�\n",superblock->s_BLOCK_SIZE,superblock->s_SUPERBLOCK_SIZE);//������ռ block��С����ʵ��С���������С��
	printf("���̷ֲ���\n");
	printf("\t�����鿪ʼλ�ã�%d B\n",superblock->s_Superblock_StartAddr);//�����鿪ʼλ��
	printf("\tinodeλͼ��ʼλ�ã�%d B\n",superblock->s_InodeBitmap_StartAddr);//inodeλͼ��ʼλ��
	printf("\tblockλͼ��ʼλ�ã�%d B\n",superblock->s_BlockBitmap_StartAddr);//blockλͼ��ʼλ��
	printf("\tinode����ʼλ�ã�%d B\n",superblock->s_Inode_StartAddr);//inode����ʼλ��
	printf("\tblock����ʼλ�ã�%d B\n",superblock->s_Block_StartAddr);//
	printf("\n");

	return ;
}

void printInodeBitmap()	//��ӡinodeʹ�����
{
	printf("\n");
	printf("inodeʹ�ñ�[uesd:%d %d/%d]\n",superblock->s_INODE_NUM-superblock->s_free_INODE_NUM,superblock->s_free_INODE_NUM,superblock->s_INODE_NUM);//inodeʹ�ñ�[uesd:inode��-����inode��/inode��]
	int i;
	i = 0;
	printf("0 ");
	while(i<superblock->s_INODE_NUM){//��ӡinodeʹ�����
		if(inode_bitmap[i])//inodeʹ��
			printf("*");
		else
			printf(".");
		i++;
		if(i!=0 && i%32==0){
			printf("\n");
			if(i!=superblock->s_INODE_NUM)//���i!=inode�������ӡ�ո�
				printf("%d ",i/32);
		}
	}
	printf("\n");
	printf("\n");
	return ;
}

void printBlockBitmap(int num)	//��ӡblockʹ�����
{
	printf("\n");
	printf("block�����̿飩ʹ�ñ�[used:%d %d/%d]\n",superblock->s_BLOCK_NUM-superblock->s_free_BLOCK_NUM,superblock->s_free_BLOCK_NUM,superblock->s_BLOCK_NUM);//block�����̿飩ʹ�ñ�[used:block��-����block��/block��]
	int i;
	i = 0;
	printf("0 ");
	while(i<num){
		if(block_bitmap[i])//blockʹ��
			printf("*");
		else
			printf(".");
		i++;
		if(i!=0 && i%32==0){
			printf("\n");
			if(num==superblock->s_BLOCK_NUM)//���num=block�������ӡ�ո�//!�������getchar���ҳ����Ķ�ȡ
				getchar();
			if(i!=superblock->s_BLOCK_NUM)//���i!=block�������ӡi/32
				printf("%d ",i/32);
		}
	}
	printf("\n");
	printf("\n");
	return ;
}

int balloc()	//���̿���亯��
{
	//���ճ������ӷ���ζ�ȡʹ�ÿ��п�ķ�ʽȥ���з���
	//ʹ�ó������еĿ��п��ջ
	//���㵱ǰջ��
	int top;	//ջ��ָ��
	if(superblock->s_free_BLOCK_NUM==0){	//ʣ����п���Ϊ0//�������������ʵ��ʣ����п���Ϊ�㣬�򷵻�-1
		printf("û�п��п���Է���\n");
		return -1;	//û�пɷ���Ŀ��п飬����-1
	}
	else{	//����ʣ���//����ջ��ָ���λ��
		top = (superblock->s_free_BLOCK_NUM-1) % superblock->s_blocks_per_group;
	}
	//��ջ��ȡ��
	//�������ջ�ף�����ǰ��ŵ�ַ���أ���Ϊջ�׿�ţ�����ջ��ָ����¿��п��ջ����ԭ����ջ
	int retAddr;
	//�����ǻ��п��п���������������η����
	if(top==0){
		retAddr = superblock->s_free_addr;//����ջ���ĵ�ַ
		superblock->s_free_addr = superblock->s_free[0];	//ȡ����һ�����п��п��ջ�Ŀ��п��λ�ã����¿��п��ջָ��
		
		//ȡ����Ӧ���п����ݣ�����ԭ���Ŀ��п��ջ

		//ȡ����һ�����п��ջ������ԭ����
		fseek(fr,superblock->s_free_addr,SEEK_SET);
		fread(superblock->s_free,sizeof(superblock->s_free),1,fr);
		fflush(fr);

		superblock->s_free_BLOCK_NUM--;//���³������п��п���

	}
	else{	//�����Ϊջ�ף���ջ��ָ��ĵ�ַ���أ�ջ��ָ��-1.
		retAddr = superblock->s_free[top];	//���淵�ص�ַ
		superblock->s_free[top] = -1;	//��ջ��
		top--;		//ջ��ָ��-1
		superblock->s_free_BLOCK_NUM--;	//���п���-1

	}

	//���³�����
	fseek(fw,Superblock_StartAddr,SEEK_SET);//���ļ�ָ���ƶ����������λ��
	fwrite(superblock,sizeof(SuperBlock),1,fw);//��������д���ļ�
	fflush(fw);//������������д���ļ�

	//����blockλͼ
	block_bitmap[(retAddr-Block_StartAddr)/BLOCK_SIZE] = 1;
	fseek(fw,(retAddr-Block_StartAddr)/BLOCK_SIZE+BlockBitmap_StartAddr,SEEK_SET);	//(retAddr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
	fwrite(&block_bitmap[(retAddr-Block_StartAddr)/BLOCK_SIZE],sizeof(bool),1,fw);//�����п�λͼд���ļ�
	fflush(fw);//������������д���ļ�

	return retAddr;

}

bool bfree(int addr)	//���̿��ͷź���
{
	//�ж�
	//�õ�ַ���Ǵ��̿����ʼ��ַ
	if( (addr-Block_StartAddr) % superblock->s_BLOCK_SIZE != 0 ){
		printf("��ַ����,��λ�ò���block�����̿飩��ʼλ��\n");
		return false;
	}
	unsigned int bno = (addr-Block_StartAddr) / superblock->s_BLOCK_SIZE;	//inode�ڵ��
	//�õ�ַ��δʹ�ã������ͷſռ�
	if(block_bitmap[bno]==0){
		printf("��block�����̿飩��δʹ�ã��޷��ͷ�\n");//�õ�ַ��δʹ�ã������ͷſռ�
		return false;
	}	
	//����ʱ�����ͷŴ��̿�������һ���ǳ������ӷ���һ������
	//�����ͷ�
	//���㵱ǰջ��
	int top;	//ջ��ָ��
	if(superblock->s_free_BLOCK_NUM==superblock->s_BLOCK_NUM){	//û�зǿ��еĴ��̿�
		printf("û�зǿ��еĴ��̿飬�޷��ͷ�\n");
		return false;	//û�пɷ���Ŀ��п飬����-1
	}
	else{	//����
		top = (superblock->s_free_BLOCK_NUM-1) % superblock->s_blocks_per_group;
					
		//���block����
		char tmp[BLOCK_SIZE] = {0};
		fseek(fw,addr,SEEK_SET);	
		fwrite(tmp,sizeof(tmp),1,fw);

		if(top == superblock->s_blocks_per_group-1){	//��ջ����

			//�ÿ��п���Ϊ�µĿ��п��ջ
			superblock->s_free[0] = superblock->s_free_addr;	//�µĿ��п��ջ��һ����ַָ��ɵĿ��п��ջָ��ӵ���������������
			int i;
			for(i=1;i<superblock->s_blocks_per_group;i++){
				superblock->s_free[i] = -1;	//���ջԪ�ص�������ַ
			}
			fseek(fw,addr,SEEK_SET);//���ļ�ָ���ƶ������п��ջ����ʼ��ַ
			fwrite(superblock->s_free,sizeof(superblock->s_free),1,fw);	//����������̿飬512�ֽ�
			
		}
		else{	//ջ��δ��
			top++;	//ջ��ָ��+1
			superblock->s_free[top] = addr;	//ջ���������Ҫ�ͷŵĵ�ַ����Ϊ�µĿ��п�
		}
	}
	

	//���³�����
	superblock->s_free_BLOCK_NUM++;	//���п���+1
	fseek(fw,Superblock_StartAddr,SEEK_SET);//���ļ�ָ���ƶ����������λ��
	fwrite(superblock,sizeof(SuperBlock),1,fw);//��������д���ļ�

	//����blockλͼ
	block_bitmap[bno] = 0;
	fseek(fw,bno+BlockBitmap_StartAddr,SEEK_SET);	//(addr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
	fwrite(&block_bitmap[bno],sizeof(bool),1,fw);//�����п�λͼд���ļ�
	fflush(fw);//������������д���ļ�

	return true;
}

int ialloc()	//����i�ڵ�������������inode��ַ
{
	//��inodeλͼ��˳����ҿ��е�inode���ҵ��򷵻�inode��ַ������������
	if(superblock->s_free_INODE_NUM==0){
		printf("û�п���inode���Է���\n");
		return -1;
	}
	else{//L˳��Ĳ��ҵ����е�inode���ز��ҽ��з����ͬʱȥ���³������Լ�inodeλͼ
			
		//˳����ҿ��е�inode
		int i;
		for(i=0;i<superblock->s_INODE_NUM;i++){
			if(inode_bitmap[i]==0)	//�ҵ�����inode
				break;
		}


		//���³�����
		superblock->s_free_INODE_NUM--;	//����inode��-1
		fseek(fw,Superblock_StartAddr,SEEK_SET);
		fwrite(superblock,sizeof(SuperBlock),1,fw);

		//����inodeλͼ
		inode_bitmap[i] = 1;
		fseek(fw,InodeBitmap_StartAddr+i,SEEK_SET);	
		fwrite(&inode_bitmap[i],sizeof(bool),1,fw);
		fflush(fw);

		return Inode_StartAddr + i*superblock->s_INODE_SIZE;
	}
}

bool ifree(int addr)	//�ͷ�i���������
{
	//�ж� ����ṩ�ĵ�ַ���� ����
	if( (addr-Inode_StartAddr) % superblock->s_INODE_SIZE != 0 ){
		printf("��ַ����,��λ�ò���i�ڵ���ʼλ��\n");
		return false;
	}
	unsigned short ino = (addr-Inode_StartAddr) / superblock->s_INODE_SIZE;	//inode�ڵ��
	if(inode_bitmap[ino]==0){//�����inode��û���ͷ�����Ҫ�����ͷŵ�һ������
		printf("��inode��δʹ�ã��޷��ͷ�\n");
		return false;
	}	
	
	//���inode����
	Inode tmp = {0};
	fseek(fw,addr,SEEK_SET);	//���ļ�ָ���ƶ���inode����ʼ��ַ
	fwrite(&tmp,sizeof(tmp),1,fw);//��inode�������

	//���³�����
	superblock->s_free_INODE_NUM++;	//����inode��+1
	//����inode��+1
	fseek(fw,Superblock_StartAddr,SEEK_SET);//	���ļ�ָ���ƶ������������ʼ��ַ
	fwrite(superblock,sizeof(SuperBlock),1,fw);//��������д���ļ�

	//����inodeλͼ
	inode_bitmap[ino] = 0;
	fseek(fw,InodeBitmap_StartAddr+ino,SEEK_SET);	//	���ļ�ָ���ƶ���inodeλͼ����ʼ��ַ
	fwrite(&inode_bitmap[ino],sizeof(bool),1,fw);//��inodeλͼд���ļ�
	fflush(fw);//������������д���ļ�

	return true;
}

bool mkdir(int parinoAddr,char name[])	//Ŀ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼��
{
	if(strlen(name)>=MAX_NAME_SIZE){//���Ŀ¼�����ȳ�����󳤶��򷵻�
		printf("�������Ŀ¼������\n");
		return false;
	}

	DirItem dirlist[16];	//��ʱĿ¼�嵥

	//�������ַȡ��inode
	Inode cur;
	fseek(fr,parinoAddr,SEEK_SET);	//���ļ�ָ���ƶ���inode����ʼ��ַ
	fread(&cur,sizeof(Inode),1,fr);//��inode���ݶ���
	
	int i = 0;
	int cnt = cur.i_cnt+1;	//Ŀ¼����
	int posi = -1,posj = -1;
	while(i<160){
		//160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
		int dno = i/16;	//�ڵڼ���ֱ�ӿ���

		if(cur.i_dirBlock[dno]==-1){
			i+=16;
			continue;
		}
		//ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
		fseek(fr,cur.i_dirBlock[dno],SEEK_SET);	//���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ
		fread(dirlist,sizeof(dirlist),1,fr);//��ֱ�ӿ����ݶ���
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for(j=0;j<16;j++){

			if( strcmp(dirlist[j].itemName,name)==0 ){
				Inode tmp;
				fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	//���ļ�ָ���ƶ���inode����ʼ��ַ
				fread(&tmp,sizeof(Inode),1,fr);//��inode���ݶ���
				if( ((tmp.i_mode>>9)&1)==1 ){	//����Ŀ¼��>>9��֪��ʲô��˼
					printf("Ŀ¼�Ѵ���\n");
					return false;
				}
			}
			else if( strcmp(dirlist[j].itemName,"")==0 ){//��Ŀ¼��
				//�ҵ�һ�����м�¼������Ŀ¼���������λ�� 
				//��¼���λ��
				if(posi==-1){
					posi = dno;//ֱ�ӿ��
					posj = j;//Ŀ¼���
				}

			}
			i++;
		}

	}
	/*  δд�� */

	if(posi!=-1){	//�ҵ��������λ��

		//ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
		fseek(fr,cur.i_dirBlock[posi],SEEK_SET);	//���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ
		fread(dirlist,sizeof(dirlist),1,fr);//��ֱ�ӿ����ݶ���
		fflush(fr);//������������д���ļ�

		//�������Ŀ¼��
		strcpy(dirlist[posj].itemName,name);	//Ŀ¼��
		//д��������¼ "." ".."���ֱ�ָ��ǰinode�ڵ��ַ���͸�inode�ڵ�
		int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ 
		if(chiinoAddr==-1){
			printf("inode����ʧ��\n");
			return false;
		}
		dirlist[posj].inodeAddr = chiinoAddr; //������µ�Ŀ¼�����inode��ַ

			//��������Ŀ��inode
			Inode p;
			p.i_ino = (chiinoAddr-Inode_StartAddr)/superblock->s_INODE_SIZE;
			p.i_atime = time(NULL);//������ʱ��
			p.i_ctime = time(NULL);//����ʱ��
			p.i_mtime = time(NULL);//����޸�ʱ��
			strcpy(p.i_uname,Cur_User_Name);
			strcpy(p.i_gname,Cur_Group_Name);
			p.i_cnt = 2;	//�������ǰĿ¼,"."��".."

				//�������inode�Ĵ��̿飬�ڴ��̺���д��������¼ . �� ..
				int curblockAddr = balloc();//������̿�
				if(curblockAddr==-1){
					printf("block����ʧ��\n");
					return false;
				}
				DirItem dirlist2[16] = {0};	//��ʱĿ¼���б� - 2
				strcpy(dirlist2[0].itemName,".");
				strcpy(dirlist2[1].itemName,"..");
				dirlist2[0].inodeAddr = chiinoAddr;	//��ǰĿ¼inode��ַ
				dirlist2[1].inodeAddr = parinoAddr;	//��Ŀ¼inode��ַ

				//д�뵽��ǰĿ¼�Ĵ��̿�
				fseek(fw,curblockAddr,SEEK_SET);	//	���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ
				fwrite(dirlist2,sizeof(dirlist2),1,fw);//��ֱ�ӿ�����д���ļ�

			p.i_dirBlock[0] = curblockAddr;
			int k;
			for(k=1;k<10;k++){
				p.i_dirBlock[k] = -1;//����ֱ�ӿ���0
			}
			p.i_size = superblock->s_BLOCK_SIZE;//�ļ���С
			p.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
			p.i_mode = MODE_DIR | DIR_DEF_PERMISSION;

			//��inodeд�뵽�����inode��ַ
			fseek(fw,chiinoAddr,SEEK_SET);	
			fwrite(&p,sizeof(Inode),1,fw);

		//����ǰĿ¼�Ĵ��̿�д��
		fseek(fw,cur.i_dirBlock[posi],SEEK_SET);//���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ	
		fwrite(dirlist,sizeof(dirlist),1,fw);//��ֱ�ӿ�����д���ļ�

		//д��inode
		cur.i_cnt++;
		fseek(fw,parinoAddr,SEEK_SET);	//	���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ
		fwrite(&cur,sizeof(Inode),1,fw);//��ֱ�ӿ�����д���ļ�
		fflush(fw);
		
		return true;
	}
	else{
		printf("û�ҵ�����Ŀ¼��,Ŀ¼����ʧ��");
		return false;
	}
}

void rmall(int parinoAddr)	//ɾ���ýڵ��������ļ���Ŀ¼
{
	//�������ַȡ��inode
	Inode cur;
	fseek(fr,parinoAddr,SEEK_SET);//���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ
	fread(&cur,sizeof(Inode),1,fr);//��ֱ�ӿ�����д���ļ�

	//ȡ��Ŀ¼����
	int cnt = cur.i_cnt;
	if(cnt<=2){
		bfree(cur.i_dirBlock[0]);//	�ͷ�ֱ�ӿ�
		ifree(parinoAddr);//�ͷ�inode
		return ;
	}

	//����ȡ�����̿�
	int i = 0;
	while(i<160){	//С��160
		DirItem dirlist[16] = {0};

		if(cur.i_dirBlock[i/16]==-1){//�����ֱ�ӿ�Ϊ�գ�������
			i+=16;
			continue;
		}
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i/16];
		fseek(fr,parblockAddr,SEEK_SET);	//	���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ
		fread(&dirlist,sizeof(dirlist),1,fr);//��ֱ�ӿ�����д���ļ�

		//�Ӵ��̿�������ȡ��Ŀ¼��ݹ�ɾ��
		int j;
		bool f = false;
		for(j=0;j<16;j++){
			//Inode tmp;

			if( ! (strcmp(dirlist[j].itemName,".")==0 || 
						strcmp(dirlist[j].itemName,"..")==0 || 
						strcmp(dirlist[j].itemName,"")==0 ) ){//������ǿ�Ŀ¼��
				f = true;
				rmall(dirlist[j].inodeAddr);	//�ݹ�ɾ��//ͨ���ݹ�ķ�ʽ���ڵ����ɾ��
			}

			cnt = cur.i_cnt;
			i++;
		}

		//�ô��̿��ѿգ�����
		if(f)
			bfree(parblockAddr);//����֮�󽫴��̿� ���л���

	}
	//��inode�ѿգ�����
	ifree(parinoAddr);//����֮��inode ���л���
	return ;

}

bool rmdir(int parinoAddr,char name[])	//Ŀ¼ɾ������
{
	if(strlen(name)>=MAX_NAME_SIZE){
		printf("�������Ŀ¼������\n");
		return false;
	}

	if(strcmp(name,".")==0 || strcmp(name,"..")==0){
		printf("�������\n");
		return 0;
	}

	//�������ַȡ��inode
	Inode cur;
	fseek(fr,parinoAddr,SEEK_SET);	
	fread(&cur,sizeof(Inode),1,fr);

	//ȡ��Ŀ¼����
	int cnt = cur.i_cnt;

	//�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother//Ҫ���ж�Ȩ���Ƿ��㹻֮���ٶԶ�Ӧ��Ŀ¼����ɾ��
	int filemode;
	if(strcmp(Cur_User_Name,cur.i_uname)==0 ) 
		filemode = 6;
	else if(strcmp(Cur_User_Name,cur.i_gname)==0)
		filemode = 3;
	else 
		filemode = 0;

	if( (((cur.i_mode>>filemode>>1)&1)==0) && (strcmp(Cur_User_Name,"root")!=0) ){
		//û��д��Ȩ��
		printf("Ȩ�޲��㣺��д��Ȩ��\n");
		return false;
	}


	//����ȡ�����̿�
	int i = 0;
	while(i<160){	//С��160
		DirItem dirlist[16] = {0};

		if(cur.i_dirBlock[i/16]==-1){//���̿�Ϊ�գ�����
			i+=16;
			continue;
		}
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i/16];
		fseek(fr,parblockAddr,SEEK_SET);	//	���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ
		fread(&dirlist,sizeof(dirlist),1,fr);//��ֱ�ӿ�����д���ļ�

		//�ҵ�Ҫɾ����Ŀ¼
		int j;
		for(j=0;j<16;j++){
			Inode tmp;
			//ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
			fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	
			fread(&tmp,sizeof(Inode),1,fr);

			if( strcmp(dirlist[j].itemName,name)==0){
				if( ( (tmp.i_mode>>9) & 1 ) == 1 ){	//�ҵ�Ŀ¼
					//��Ŀ¼

					rmall(dirlist[j].inodeAddr);//Ŀ¼ɾ������

					//ɾ����Ŀ¼��Ŀ��д�ش���
					strcpy(dirlist[j].itemName,"");//����Ŀ¼��Ŀ���������
					dirlist[j].inodeAddr = -1;
					fseek(fw,parblockAddr,SEEK_SET);//���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ
					fwrite(&dirlist,sizeof(dirlist),1,fw);//��ֱ�ӿ�����д���ļ�
					cur.i_cnt--;//Ŀ¼������һ
					fseek(fw,parinoAddr,SEEK_SET);	
					fwrite(&cur,sizeof(Inode),1,fw);

					fflush(fw);
					return true;
				}
				else{
					//����Ŀ¼������
				}
			}
			i++;
		}

	}
	
	printf("û���ҵ���Ŀ¼\n");
	return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool create(int parinoAddr,char name[],char buf[])	//�����ļ��������ڸ�Ŀ¼�´����ļ����ļ����ݴ���buf
{
	if(strlen(name)>=MAX_NAME_SIZE){
		printf("��������ļ�������\n");
		return false;
	}

	DirItem dirlist[16];	//��ʱĿ¼�嵥

	//�������ַȡ��inode
	Inode cur;
	fseek(fr,parinoAddr,SEEK_SET);	
	fread(&cur,sizeof(Inode),1,fr);
	
	int i = 0;
	int posi = -1,posj = -1;	//�ҵ���Ŀ¼λ��
	int dno;
	int cnt = cur.i_cnt+1;	//Ŀ¼����
	while(i<160){
		//160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
		dno = i/16;	//�ڵڼ���ֱ�ӿ���

		if(cur.i_dirBlock[dno]==-1){//���̿�Ϊ�գ�����
			i+=16;
			continue;
		}
		fseek(fr,cur.i_dirBlock[dno],SEEK_SET);	
		fread(dirlist,sizeof(dirlist),1,fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for(j=0;j<16;j++){

			if( posi==-1 && strcmp(dirlist[j].itemName,"")==0 ){//�ҵ���λ��
				//�ҵ�һ�����м�¼�������ļ����������λ�� 
				posi = dno;//ֱ�ӿ��
				posj = j;//Ŀ¼���
			}
			else if(strcmp(dirlist[j].itemName,name)==0 ){//�ҵ�ͬ���ļ�
				//������ȡ��inode���ж��Ƿ����ļ�
				Inode cur2;
				fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	
				fread(&cur2,sizeof(Inode),1,fr);
				if( ((cur2.i_mode>>9)&1)==0 ){	//���ļ������������ܴ����ļ�
					printf("�ļ��Ѵ���\n");
					buf[0] = '\0';
					return false;
				}
			}
			i++;
		}

	}
	if(posi!=-1){	//֮ǰ�ҵ�һ��Ŀ¼����
		//ȡ��֮ǰ�Ǹ�����Ŀ¼���Ӧ�Ĵ��̿�
		fseek(fr,cur.i_dirBlock[posi],SEEK_SET);//���ļ�ָ���ƶ���ֱ�ӿ����ʼ��ַ	
		fread(dirlist,sizeof(dirlist),1,fr);//��ֱ�ӿ�����д���ļ�
		fflush(fr);

		//�������Ŀ¼��
		strcpy(dirlist[posj].itemName,name);	//�ļ���
		int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ 
		if(chiinoAddr==-1){
			printf("inode����ʧ��\n");
			return false;
		}
		dirlist[posj].inodeAddr = chiinoAddr; //������µ�Ŀ¼�����inode��ַ

			//��������Ŀ��inode
			Inode p;
			p.i_ino = (chiinoAddr-Inode_StartAddr)/superblock->s_INODE_SIZE;//inode��
			p.i_atime = time(NULL);//������ʱ��
			p.i_ctime = time(NULL);//����ʱ��
			p.i_mtime = time(NULL);//����޸�ʱ��
			strcpy(p.i_uname,Cur_User_Name);
			strcpy(p.i_gname,Cur_Group_Name);
			p.i_cnt = 1;	//ֻ��һ���ļ�ָ��


				//��buf���ݴ浽���̿� 
				int k;
				int len = strlen(buf);	//�ļ����ȣ���λΪ�ֽ�
				for(k=0;k<len;k+=superblock->s_BLOCK_SIZE){	//���10�Σ�10�����̿죬�����5K//�޶��������ļ��Ĵ�С
					//�������inode�Ĵ��̿飬�ӿ���̨��ȡ����
					int curblockAddr = balloc();//����һ�����̿�
					if(curblockAddr==-1){
						printf("block����ʧ��\n");
						return false;
					}
					p.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
					//д�뵽��ǰĿ¼�Ĵ��̿�
					fseek(fw,curblockAddr,SEEK_SET);	
					fwrite(buf+k,superblock->s_BLOCK_SIZE,1,fw);
				}
				

			for(k=len/superblock->s_BLOCK_SIZE+1;k<10;k++){
				p.i_dirBlock[k] = -1;
			}
			if(len==0){	//����Ϊ0�Ļ�Ҳ�ָ���һ��block
				int curblockAddr = balloc();
				if(curblockAddr==-1){
					printf("block����ʧ��\n");
					return false;
				}
				p.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
				//д�뵽��ǰĿ¼�Ĵ��̿�
				fseek(fw,curblockAddr,SEEK_SET);	
				fwrite(buf,superblock->s_BLOCK_SIZE,1,fw);

			}
			p.i_size = len;//�ļ�����
			p.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
			p.i_mode = 0;
			p.i_mode = MODE_FILE | FILE_DEF_PERMISSION;//�ļ�Ȩ��

			//��inodeд�뵽�����inode��ַ
			fseek(fw,chiinoAddr,SEEK_SET);	
			fwrite(&p,sizeof(Inode),1,fw);

		//����ǰĿ¼�Ĵ��̿�д��
		fseek(fw,cur.i_dirBlock[posi],SEEK_SET);	
		fwrite(dirlist,sizeof(dirlist),1,fw);

		//д��inode
		cur.i_cnt++;
		fseek(fw,parinoAddr,SEEK_SET);	
		fwrite(&cur,sizeof(Inode),1,fw);
		fflush(fw);

		return true;
	}
	else
		return false;
}
//�ú������ҵ���Ӧ���ļ�֮��ֻ����ȥɾ����Ӧ�ĺ��� ��������Ŀ¼�Ļ�������в���
bool del(int parinoAddr,char name[])		//ɾ���ļ��������ڵ�ǰĿ¼��ɾ���ļ�
{
	
	if(strlen(name)>=MAX_NAME_SIZE){
		printf("�������Ŀ¼������\n");
		return false;
	}

	//�������ַȡ��inode
	Inode cur;
	fseek(fr,parinoAddr,SEEK_SET);	
	fread(&cur,sizeof(Inode),1,fr);

	//ȡ��Ŀ¼����
	int cnt = cur.i_cnt;

	//�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother//�ж��ļ�ģʽ�����ж��û��Ƿ����Ȩ��
	int filemode;
	if(strcmp(Cur_User_Name,cur.i_uname)==0 ) 
		filemode = 6;
	else if(strcmp(Cur_User_Name,cur.i_gname)==0)
		filemode = 3;
	else 
		filemode = 0;

	if( ((cur.i_mode>>filemode>>1)&1)==0 ){
		//û��д��Ȩ��
		printf("Ȩ�޲��㣺��д��Ȩ��\n");
		return false;
	}

	//����ȡ�����̿�
	int i = 0;
	while(i<160){	//С��160
		DirItem dirlist[16] = {0};

		if(cur.i_dirBlock[i/16]==-1){
			i+=16;
			continue;
		}
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i/16];
		fseek(fr,parblockAddr,SEEK_SET);	
		fread(&dirlist,sizeof(dirlist),1,fr);

		//�ҵ�Ҫɾ����Ŀ¼
		int pos;
		for(pos=0;pos<16;pos++){
			Inode tmp;
			//ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
			fseek(fr,dirlist[pos].inodeAddr,SEEK_SET);	
			fread(&tmp,sizeof(Inode),1,fr);

			if( strcmp(dirlist[pos].itemName,name)==0){
				if( ( (tmp.i_mode>>9) & 1 ) == 1 ){	//�ҵ�Ŀ¼
					//��Ŀ¼������
				}
				else{
					//���ļ�

					//�ͷ�block
					int k;
					for(k=0;k<10;k++)
						if(tmp.i_dirBlock[k]!=-1)
							bfree(tmp.i_dirBlock[k]);

					//�ͷ�inode
					ifree(dirlist[pos].inodeAddr);

					//ɾ����Ŀ¼��Ŀ��д�ش���
					strcpy(dirlist[pos].itemName,"");
					dirlist[pos].inodeAddr = -1;
					fseek(fw,parblockAddr,SEEK_SET);	
					fwrite(&dirlist,sizeof(dirlist),1,fw);
					cur.i_cnt--;
					fseek(fw,parinoAddr,SEEK_SET);	
					fwrite(&cur,sizeof(Inode),1,fw);

					fflush(fw);
					return true;
				}
			}
			i++;
		}

	}
	
	printf("û���ҵ����ļ�!\n");
	return false;
}


void ls(int parinoAddr)		//��ʾ��ǰĿ¼�µ������ļ����ļ��С���������ǰĿ¼��inode�ڵ��ַ 
{
	Inode cur;
	//ȡ�����inode
	fseek(fr,parinoAddr,SEEK_SET);	
	fread(&cur,sizeof(Inode),1,fr);
	fflush(fr);

	//ȡ��Ŀ¼����
	int cnt = cur.i_cnt;
	//�鿴Ȩ���Լ�ȡ����Ӧ�Ĵ��̿�
	//�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
	int filemode;
	if(strcmp(Cur_User_Name,cur.i_uname)==0 ) 
		filemode = 6;
	else if(strcmp(Cur_User_Name,cur.i_gname)==0)
		filemode = 3;
	else 
		filemode = 0;

	if( ((cur.i_mode>>filemode>>2)&1)==0 ){
		//û�ж�ȡȨ��
		printf("Ȩ�޲��㣺�޶�ȡȨ��\n");
		return ;
	}

	//����ȡ�����̿�
	int i = 0;
	while(i<cnt && i<160){
		DirItem dirlist[16] = {0};
		if(cur.i_dirBlock[i/16]==-1){
			i+=16;
			continue;
		}
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i/16];
		fseek(fr,parblockAddr,SEEK_SET);	
		fread(&dirlist,sizeof(dirlist),1,fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for(j=0;j<16 && i<cnt;j++){
			Inode tmp;
			//ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
			fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	
			fread(&tmp,sizeof(Inode),1,fr);
			fflush(fr);

			if( strcmp(dirlist[j].itemName,"")==0 ){
				continue;
			}

			//�����Ϣ
			if( ( (tmp.i_mode>>9) & 1 ) == 1 ){//��Ŀ¼
				printf("d");
			}
			else{
				printf("-");//���ļ�
			}

			tm *ptr;	//�洢ʱ��
			ptr=gmtime(&tmp.i_mtime);

			//���Ȩ����Ϣ
			int t = 8;
			while(t>=0){
				if( ((tmp.i_mode>>t)&1)==1){
					if(t%3==2)	printf("r");//��
					if(t%3==1)	printf("w");//д
					if(t%3==0)	printf("x");//ִ��
				}
				else{
					printf("-");
				}
				t--;
			}
			printf("\t");

			//����
			printf("%d\t",tmp.i_cnt);	//����
			printf("%s\t",tmp.i_uname);	//�ļ������û���
			printf("%s\t",tmp.i_gname);	//�ļ������û���
			printf("%d B\t",tmp.i_size);	//�ļ���С
			printf("%d.%d.%d %02d:%02d:%02d  ",1900+ptr->tm_year,ptr->tm_mon+1,ptr->tm_mday,(8+ptr->tm_hour)%24,ptr->tm_min,ptr->tm_sec);	//��һ���޸ĵ�ʱ��
			printf("%s",dirlist[j].itemName);	//�ļ���
			printf("\n");
			i++;
		}

	}
	/*  δд�� */

}

void cd(int parinoAddr,char name[])	//���뵱ǰĿ¼�µ�nameĿ¼
{
	//ȡ����ǰĿ¼��inode
	Inode cur;
	fseek(fr,parinoAddr,SEEK_SET);	
	fread(&cur,sizeof(Inode),1,fr);

	//����ȡ��inode��Ӧ�Ĵ��̿飬������û������Ϊname��Ŀ¼��
	int i = 0;

	//ȡ��Ŀ¼����
	int cnt = cur.i_cnt;
	//�鿴Ȩ���Լ�ȡ����Ӧ�Ĵ��̿�
	//�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
	int filemode;
	if(strcmp(Cur_User_Name,cur.i_uname)==0 ) 
		filemode = 6;
	else if(strcmp(Cur_User_Name,cur.i_gname)==0)
		filemode = 3;
	else 
		filemode = 0;

	while(i<160){
		DirItem dirlist[16] = {0};
		if(cur.i_dirBlock[i/16]==-1){
			i+=16;
			continue;
		}
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i/16];
		fseek(fr,parblockAddr,SEEK_SET);	
		fread(&dirlist,sizeof(dirlist),1,fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for(j=0;j<16;j++){
			if(strcmp(dirlist[j].itemName,name)==0){
				Inode tmp;
				//ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
				fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	
				fread(&tmp,sizeof(Inode),1,fr);

				if( ( (tmp.i_mode>>9) & 1 ) == 1 ){
					//�ҵ���Ŀ¼���ж��Ƿ���н���Ȩ��
					if( ((tmp.i_mode>>filemode>>0)&1)==0 && strcmp(Cur_User_Name,"root")!=0 ){	//root�û�����Ŀ¼�����Բ鿴 
						//û��ִ��Ȩ��
						printf("Ȩ�޲��㣺��ִ��Ȩ��\n");
						return ;
					}

					//�ҵ���Ŀ¼������Ŀ¼��������ǰĿ¼

					Cur_Dir_Addr = dirlist[j].inodeAddr;
					if( strcmp(dirlist[j].itemName,".")==0){//��ǰĿ¼
						//��Ŀ¼������
					}
					else if(strcmp(dirlist[j].itemName,"..")==0){//��һ��Ŀ¼
						//��һ��Ŀ¼
						int k;
						for(k=strlen(Cur_Dir_Name);k>=0;k--)
							if(Cur_Dir_Name[k]=='/')//�ҵ���һ��Ŀ¼
								break;
						Cur_Dir_Name[k]='\0';//�ضϵ��ļ��ṹ�ܹ�֪��ǰ�����ʲô
						if(strlen(Cur_Dir_Name)==0)//��ǰĿ¼Ϊ��Ŀ¼
							Cur_Dir_Name[0]='/',Cur_Dir_Name[1]='\0';//��Ŀ¼
					}
					else{
						if(Cur_Dir_Name[strlen(Cur_Dir_Name)-1]!='/')//��ǰĿ¼���Ǹ�Ŀ¼
							strcat(Cur_Dir_Name,"/");
						strcat(Cur_Dir_Name,dirlist[j].itemName);//������ǰĿ¼
					}

					return ;
				}
				else{
					//�ҵ���Ŀ¼��������Ŀ¼��������
				}
				
			}

			i++;
		}

	}
	
	//û�ҵ�
	printf("û�и�Ŀ¼\n");
	return ;

}

void gotoxy(HANDLE hOut, int x, int y)	//�ƶ���굽ָ��λ��
{
      COORD pos;
      pos.X = x;             //������
      pos.Y = y;            //������
      SetConsoleCursorPosition(hOut, pos);
}

void vi(int parinoAddr,char name[],char buf[])	//ģ��һ����vi�������ı���nameΪ�ļ���
{
	//���ж��ļ��Ƿ��Ѵ��ڡ�������ڣ�������ļ����༭
	if(strlen(name)>=MAX_NAME_SIZE){
		printf("��������ļ�������\n");
		return ;
	}

	//��ջ�����
	memset(buf,0,sizeof(buf));
	int maxlen = 0;	//���������󳤶�

	//��������ͬ���ļ����еĻ�����༭ģʽ��û�н��봴���ļ�ģʽ
	DirItem dirlist[16];	//��ʱĿ¼�嵥

	//�������ַȡ��inode
	Inode cur,fileInode;
	fseek(fr,parinoAddr,SEEK_SET);	
	fread(&cur,sizeof(Inode),1,fr);

	//�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
	int filemode;
	if(strcmp(Cur_User_Name,cur.i_uname)==0 ) 
		filemode = 6;
	else if(strcmp(Cur_User_Name,cur.i_gname)==0)
		filemode = 3;
	else 
		filemode = 0;
	
	int i = 0,j;
	int dno;
	int fileInodeAddr = -1;	//�ļ���inode��ַ
	bool isExist = false;	//�ļ��Ƿ��Ѵ���
	while(i<160){
		//160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
		dno = i/16;	//�ڵڼ���ֱ�ӿ���

		if(cur.i_dirBlock[dno]==-1){
			i+=16;
			continue;
		}
		fseek(fr,cur.i_dirBlock[dno],SEEK_SET);	
		fread(dirlist,sizeof(dirlist),1,fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		for(j=0;j<16;j++){
			if(strcmp(dirlist[j].itemName,name)==0 ){
				//������ȡ��inode���ж��Ƿ����ļ�
				fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	
				fread(&fileInode,sizeof(Inode),1,fr);
				if( ((fileInode.i_mode>>9)&1)==0 ){	//���ļ���������������ļ������༭	
					fileInodeAddr = dirlist[j].inodeAddr;
					isExist = true;
					goto label;
				}
			}
			i++;
		}
	}
label:

	//��ʼ��vi
	int cnt = 0;
	system("cls");	//����

	int winx,winy,curx,cury;

	HANDLE handle_out;                              //����һ�����  
	CONSOLE_SCREEN_BUFFER_INFO screen_info;         //���崰�ڻ�������Ϣ�ṹ��  
	COORD pos = {0, 0};                             //����һ������ṹ��

	if(isExist){	//�ļ��Ѵ��ڣ�����༭ģʽ�������֮ǰ���ļ�����

		//Ȩ���жϡ��ж��ļ��Ƿ�ɶ�
		if( ((fileInode.i_mode>>filemode>>2)&1)==0){
			//���ɶ�
			printf("Ȩ�޲��㣺û�ж�Ȩ��\n");
			return ;
		}

		//���ļ����ݶ�ȡ��������ʾ�ڣ�������
		i = 0;
		int sumlen = fileInode.i_size;	//�ļ�����
		int getlen = 0;	//ȡ�����ĳ���
		for(i=0;i<10;i++){
			char fileContent[1000] = {0};
			if(fileInode.i_dirBlock[i]==-1){
				continue;
			}
			//����ȡ�����̿������
			fseek(fr,fileInode.i_dirBlock[i],SEEK_SET);	
			fread(fileContent,superblock->s_BLOCK_SIZE,1,fr);	//��ȡ��һ�����̿��С������
			fflush(fr);
			//����ַ���
			int curlen = 0;	//��ǰָ��
			while(curlen<superblock->s_BLOCK_SIZE){
				if(getlen>=sumlen)	//ȫ��������
					break;
				printf("%c",fileContent[curlen]);	//�������Ļ 
				buf[cnt++] = fileContent[curlen];	//�����buf
				curlen++;
				getlen++;
			}
			if(getlen>=sumlen)
				break;
		}
		maxlen = sumlen;
	}  

	//������֮��Ĺ��λ��
	handle_out = GetStdHandle(STD_OUTPUT_HANDLE);   //��ñ�׼����豸���  
	GetConsoleScreenBufferInfo(handle_out, &screen_info);   //��ȡ������Ϣ  
	winx = screen_info.srWindow.Right - screen_info.srWindow.Left + 1;
	winy = screen_info.srWindow.Bottom - screen_info.srWindow.Top + 1;
	curx = screen_info.dwCursorPosition.X;
	cury = screen_info.dwCursorPosition.Y;


	//����vi
	//����vi��ȡ�ļ�����

	int mode = 0;	//viģʽ��һ��ʼ������ģʽ
	unsigned char c;
	while(1){
		if(mode==0){	//������ģʽ
			c=getch();
			
			if(c=='i' || c=='a'){	//����ģʽ
				if(c=='a'){
					curx++;
					if(curx==winx){
						curx = 0;
						cury++;
					
						/*
						if(cury>winy-2 || cury%(winy-1)==winy-2){
							//������һ�������·�ҳ
							if(cury%(winy-1)==winy-2)
								printf("\n");
							SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
							int i;
							for(i=0;i<winx-1;i++)
								printf(" ");
							gotoxy(handle_out,0,cury+1);
							printf(" - ����ģʽ - ");
							gotoxy(handle_out,0,cury);
						}
						*/
					}
				}
				
				if(cury>winy-2 || cury%(winy-1)==winy-2){
					//������һ�������·�ҳ
					if(cury%(winy-1)==winy-2)
						printf("\n");
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
					int i;
					for(i=0;i<winx-1;i++)
						printf(" ");
					gotoxy(handle_out,0,cury+1);
					printf(" - ����ģʽ - ");
					gotoxy(handle_out,0,cury);
				}	
				else{
					//��ʾ "����ģʽ"
					gotoxy(handle_out,0,winy-1);
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
					int i;
					for(i=0;i<winx-1;i++)
						printf(" ");
					gotoxy(handle_out,0,winy-1);
					printf(" - ����ģʽ - ");
					gotoxy(handle_out,curx,cury);
				}

				gotoxy(handle_out,curx,cury);
				mode = 1;
			

			}
			else if(c==':'){
				//system("color 09");//�����ı�Ϊ��ɫ
				if(cury-winy+2>0)
					gotoxy(handle_out,0,cury+1);
				else
					gotoxy(handle_out,0,winy-1);
				_COORD pos;
				if(cury-winy+2>0)
					pos.X = 0,pos.Y = cury+1;
				else
					pos.X = 0,pos.Y = winy-1;
				SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
				int i;
				for(i=0;i<winx-1;i++)
					printf(" ");

				if(cury-winy+2>0)
					gotoxy(handle_out,0,cury+1);
				else
				gotoxy(handle_out,0,winy-1);

				WORD att = BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY; // �ı�����
				FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//����̨������ɫ 
				SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//�����ı���ɫ
				printf(":");
				
				char pc;
				int tcnt = 1;	//������ģʽ������ַ�����
				while( c = getch() ){
					if(c=='\r'){	//�س�
						break;
					}
					else if(c=='\b'){	//�˸񣬴�������ɾ��һ���ַ� 
						//SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
						tcnt--;
						if(tcnt==0)	
							break;
						printf("\b");
						printf(" ");
						printf("\b");
						continue;
					}
					pc = c;
					printf("%c",pc);
					tcnt++;
				}
				if(pc=='q'){
					buf[cnt] = '\0'; 
					//buf[maxlen] = '\0'; 
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
					system("cls");
					break;	//vi >>>>>>>>>>>>>> �˳�����
				}
				else{
					if(cury-winy+2>0)
						gotoxy(handle_out,0,cury+1);
					else
						gotoxy(handle_out,0,winy-1);
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
					int i;
					for(i=0;i<winx-1;i++)
						printf(" ");

					if(cury-winy+2>0)
						gotoxy(handle_out,0,cury+1);
					else
						gotoxy(handle_out,0,winy-1);
					SetConsoleTextAttribute(handle_out, FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_GREEN  );	//�����ı���ɫ
					FillConsoleOutputAttribute(handle_out, att, winx, pos, NULL);	//����̨������ɫ
					printf(" ��������");
					//getch();
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
					gotoxy(handle_out,curx,cury);
				}
			}
			else if(c==27){	//ESC��������ģʽ����״̬��
				gotoxy(handle_out,0,winy-1);
				SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
				int i;
				for(i=0;i<winx-1;i++)
					printf(" ");
				gotoxy(handle_out,curx,cury);

			}
		
		}
		else if(mode==1){	//����ģʽ
		
			gotoxy(handle_out,winx/4*3,winy-1);
			int i = winx/4*3;
			while(i<winx-1){
				printf(" ");
				i++;
			}
			if(cury>winy-2)
				gotoxy(handle_out,winx/4*3,cury+1);
			else
				gotoxy(handle_out,winx/4*3,winy-1);
			printf("[��:%d,��:%d]",curx==-1?0:curx,cury);
			gotoxy(handle_out,curx,cury);

			c = getch();
			if(c==27){	// ESC����������ģʽ
				mode = 0;
				//��״̬��
				gotoxy(handle_out,0,winy-1);
				SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
				int i;
				for(i=0;i<winx-1;i++)
					printf(" ");
				continue;
			}
			else if(c=='\b'){	//�˸�ɾ��һ���ַ�
				if(cnt==0)	//�Ѿ��˵��ʼ
					continue;
				printf("\b");
				printf(" ");
				printf("\b");
				curx--;
				cnt--;	//ɾ���ַ�
				if(buf[cnt]=='\n'){
					//Ҫɾ��������ַ��ǻس������ص���һ��
					if(cury!=0)
						cury--;
					int k;
					curx = 0;
					for(k = cnt-1;buf[k]!='\n' && k>=0;k--)
						curx++;
					gotoxy(handle_out,curx,cury);
					printf(" ");
					gotoxy(handle_out,curx,cury);
					if(cury-winy+2>=0){	//��ҳʱ
						gotoxy(handle_out,curx,0);
						gotoxy(handle_out,curx,cury+1);
						SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
						int i;
						for(i=0;i<winx-1;i++)
							printf(" ");
						gotoxy(handle_out,0,cury+1);
						printf(" - ����ģʽ - ");

					}
					gotoxy(handle_out,curx,cury);

				}
				else
					buf[cnt] = ' ';
				continue;
			}
			else if(c==224){	//�ж��Ƿ��Ǽ�ͷ
				c = getch();
				if(c==75){	//���ͷ
					if(cnt!=0){
						cnt--;
						curx--;
						if(buf[cnt]=='\n'){
							//��һ���ַ��ǻس�
							if(cury!=0)
								cury--;
							int k;
							curx = 0;
							for(k = cnt-1;buf[k]!='\n' && k>=0;k--)
								curx++;
						}
						gotoxy(handle_out,curx,cury);
					}
				}
				else if(c==77){	//�Ҽ�ͷ
					cnt++;
					if(cnt>maxlen)
						maxlen = cnt;
					curx++;
					if(curx==winx){
						curx = 0;
						cury++;

						if(cury>winy-2 || cury%(winy-1)==winy-2){
							//������һ�������·�ҳ
							if(cury%(winy-1)==winy-2)
								printf("\n");
							SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
							int i;
							for(i=0;i<winx-1;i++)
								printf(" ");
							gotoxy(handle_out,0,cury+1);
							printf(" - ����ģʽ - ");
							gotoxy(handle_out,0,cury);
						}

					}
					gotoxy(handle_out,curx,cury);
				}
				continue;
			}
			if(c=='\r'){	//�����س�
				printf("\n");
				curx = 0;
				cury++;
		
				if(cury>winy-2 || cury%(winy-1)==winy-2){
					//������һ�������·�ҳ
					if(cury%(winy-1)==winy-2)
						printf("\n");
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
					int i;
					for(i=0;i<winx-1;i++)
						printf(" ");
					gotoxy(handle_out,0,cury+1);
					printf(" - ����ģʽ - ");
					gotoxy(handle_out,0,cury);
				}

				buf[cnt++] = '\n';
				if(cnt>maxlen)
					maxlen = cnt;
				continue;
			}
			else{
				printf("%c",c);
			}
			//�ƶ����
			curx++;
			if(curx==winx){
				curx = 0;
				cury++;
	
				if(cury>winy-2 || cury%(winy-1)==winy-2){
					//������һ�������·�ҳ
					if(cury%(winy-1)==winy-2)
						printf("\n");
					SetConsoleTextAttribute(handle_out, screen_info.wAttributes); // �ָ�ԭ��������
					int i;
					for(i=0;i<winx-1;i++)
						printf(" ");
					gotoxy(handle_out,0,cury+1);
					printf(" - ����ģʽ - ");
					gotoxy(handle_out,0,cury);
				}

				buf[cnt++] = '\n';
				if(cnt>maxlen)
					maxlen = cnt;
				if(cury==winy){
					printf("\n");
				}
			}
			//��¼�ַ� 
			buf[cnt++] = c;
			if(cnt>maxlen)
				maxlen = cnt;
		}
		else{	//����ģʽ
		}
	}
	if(isExist){	//����Ǳ༭ģʽ
		//��buf����д���ļ��Ĵ��̿�

		if( ((fileInode.i_mode>>filemode>>1)&1)==1 ){	//��д
			writefile(fileInode,fileInodeAddr,buf);
		}
		else{	//����д
			printf("Ȩ�޲��㣺��д��Ȩ��\n");
		}

	}
	else{	//�Ǵ����ļ�ģʽ
		if( ((cur.i_mode>>filemode>>1)&1)==1){
			//��д�����Դ����ļ�
			create(parinoAddr,name,buf);	//�����ļ�
		}
		else{
			printf("Ȩ�޲��㣺��д��Ȩ��\n");
			return ;
		}
	}
}

void writefile(Inode fileInode,int fileInodeAddr,char buf[])	//��buf����д���ļ��Ĵ��̿�
{
	//��buf����д�ش��̿� 
	int k;
	int len = strlen(buf);	//�ļ����ȣ���λΪ�ֽ�
	for(k=0;k<len;k+=superblock->s_BLOCK_SIZE){	//���10�Σ�10�����̿죬�����5K
		//�������inode�Ĵ��̿飬�ӿ���̨��ȡ����
		int curblockAddr;
		if(fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE]==-1){
			//ȱ�ٴ��̿飬����һ��
			curblockAddr = balloc();
			if(curblockAddr==-1){
				printf("block����ʧ��\n");
				return ;
			}
			fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE] = curblockAddr;
		}
		else{
			curblockAddr = fileInode.i_dirBlock[k/superblock->s_BLOCK_SIZE];
		}
		//д�뵽��ǰĿ¼�Ĵ��̿�
		fseek(fw,curblockAddr,SEEK_SET);	
		fwrite(buf+k,superblock->s_BLOCK_SIZE,1,fw);
		fflush(fw);
	}
	//���¸��ļ���С
	fileInode.i_size = len;
	fileInode.i_mtime = time(NULL);
	fseek(fw,fileInodeAddr,SEEK_SET);	
	fwrite(&fileInode,sizeof(Inode),1,fw);
	fflush(fw);
}

void inUsername(char username[])	//�����û���
{
	printf("username:");
	scanf("%s",username);	//�û���
}

void inPasswd(char passwd[])	//��������
{
	int plen = 0;
	char c;
	fflush(stdin);	//��ջ�����
	printf("passwd:");
	while(c=getch()){
		if(c=='\r'){	//����س�������ȷ��
			passwd[plen] = '\0';
			fflush(stdin);	//�建����
			printf("\n");
			break;
		}
		else if(c=='\b'){	//�˸�ɾ��һ���ַ�
			if(plen!=0){	//û��ɾ��ͷ
				plen--;
			}
		}
		else{	//�����ַ�
			passwd[plen++] = c;
		}
	}
}

bool login()	//��½����
{
	char username[100] = {0};
	char passwd[100] = {0};
	inUsername(username);	//�����û���//L����������û������û�����
	inPasswd(passwd);		//�����û�����
	if(check(username,passwd)){	//�˶��û���������
		isLogin = true;
		return true;
	}
	else{
		isLogin = false;
		return false;
	}
}

bool check(char username[],char passwd[])	//�˶��û���������L���������ļ� ����Ϣ��ȡ�������û�������û���������������
{
	int passwd_Inode_Addr = -1;	//�û��ļ�inode��ַ
	int shadow_Inode_Addr = -1;	//�û������ļ�inode��ַ
	Inode passwd_Inode;		//�û��ļ���inode
	Inode shadow_Inode;		//�û������ļ���inode

	Inode cur_dir_inode;	//��ǰĿ¼��inode
	int i,j;
	DirItem dirlist[16];	//��ʱĿ¼

	cd(Cur_Dir_Addr,"etc");	//���������ļ�Ŀ¼//L��etc�������ļ�

	//�ҵ�passwd��shadow�ļ�inode��ַ����ȡ��
	//ȡ����ǰĿ¼��inode
	fseek(fr,Cur_Dir_Addr,SEEK_SET);	
	fread(&cur_dir_inode,sizeof(Inode),1,fr);
	//����ȡ�����̿飬����passwd�ļ���inode��ַ����shadow�ļ���inode��ַ
	for(i=0;i<10;i++){
		if(cur_dir_inode.i_dirBlock[i]==-1){
			continue;
		}
		//����ȡ�����̿�
		fseek(fr,cur_dir_inode.i_dirBlock[i],SEEK_SET);	
		fread(&dirlist,sizeof(dirlist),1,fr);

		for(j=0;j<16;j++){	//����Ŀ¼��
			if( strcmp(dirlist[j].itemName,"passwd")==0 ||	//�ҵ�passwd����shadow��Ŀ
				strcmp(dirlist[j].itemName,"shadow")==0  ){
				Inode tmp;	//��ʱinode
				//ȡ��inode���ж��Ƿ����ļ�
				fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	
				fread(&tmp,sizeof(Inode),1,fr);

				if( ((tmp.i_mode>>9)&1)==0 ){	
					//���ļ�
					//�б���passwd�ļ�����shadow�ļ�
					if( strcmp(dirlist[j].itemName,"passwd")==0 ){	
						passwd_Inode_Addr = dirlist[j].inodeAddr;
						passwd_Inode = tmp;
					}
					else if(strcmp(dirlist[j].itemName,"shadow")==0 ){
						shadow_Inode_Addr = dirlist[j].inodeAddr;
						shadow_Inode = tmp;
					}
				}
			}
		}
		if(passwd_Inode_Addr!=-1 && shadow_Inode_Addr!=-1)	//���ҵ���
			break;
	}

	//����passwd�ļ������Ƿ�����û�username
	char buf[1000000];	//���1M���ݴ�passwd���ļ�����
	char buf2[600];		//�ݴ���̿�����
	j = 0;	//���̿�ָ��
	//ȡ��passwd�ļ�����
	for(i=0;i<passwd_Inode.i_size;i++){
		if(i%superblock->s_BLOCK_SIZE==0){	//������
			//���µĴ��̿�
			fseek(fr,passwd_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);	
			fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
			j = 0;
		}
		buf[i] = buf2[j++];
	}
	buf[i] = '\0';
	if(strstr(buf,username)==NULL){
		//û�ҵ����û�
		printf("�û�������\n");
		cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼
		return false;
	}

	//������ڣ��鿴shadow�ļ���ȡ�����룬�˶�passwd�Ƿ���ȷ
	//ȡ��shadow�ļ�����
	j = 0;
	for(i=0;i<shadow_Inode.i_size;i++){
		if(i%superblock->s_BLOCK_SIZE==0){	//������������̿�
			//���µĴ��̿�
			fseek(fr,shadow_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);	
			fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
			j = 0;
		}
		buf[i] = buf2[j++];
	}
	buf[i] = '\0';

	char *p;	//�ַ�ָ��
	if( (p = strstr(buf,username))==NULL){
		//û�ҵ����û�
		printf("shadow�ļ��в����ڸ��û�\n");
		cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼
		return false;
	}
	//�ҵ����û���ȡ������
	while((*p)!=':'){
		p++;
	}
	p++;
	j = 0;
	while((*p)!='\n'){
		buf2[j++] = *p;
		p++;
	}
	buf2[j] = '\0';

	//�˶�����
	if(strcmp(buf2,passwd)==0){	//������ȷ����½
		strcpy(Cur_User_Name,username);
		if(strcmp(username,"root")==0)
			strcpy(Cur_Group_Name,"root");	//��ǰ��½�û�����
		else
			strcpy(Cur_Group_Name,"user");	//��ǰ��½�û�����
		cd(Cur_Dir_Addr,"..");
		cd(Cur_Dir_Addr,"home");\
		cd(Cur_Dir_Addr,username);	//���뵽�û�Ŀ¼
		strcpy(Cur_User_Dir_Name,Cur_Dir_Name);	//���Ƶ�ǰ��½�û�Ŀ¼��
		return true;
	}
	else{
		printf("�������\n");
		cd(Cur_Dir_Addr,"..");	//�ص���Ŀ¼
		return false;
	}
}

void gotoRoot()	//�ص���Ŀ¼
{
	memset(Cur_User_Name,0,sizeof(Cur_User_Name));		//��յ�ǰ�û���
	memset(Cur_User_Dir_Name,0,sizeof(Cur_User_Dir_Name));	//��յ�ǰ�û�Ŀ¼
	Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
	strcpy(Cur_Dir_Name,"/");		//��ǰĿ¼��Ϊ"/"
}

void logout()	//�û�ע��
{
	//�ص���Ŀ¼
	gotoRoot();

	isLogin = false;
	printf("�û�ע��\n");
	system("pause");
	system("cls");
}

bool useradd(char username[])	//�û�ע��
{
	if(strcmp(Cur_User_Name,"root")!=0){
		printf("Ȩ�޲���\n");
		return false;
	}
	int passwd_Inode_Addr = -1;	//�û��ļ�inode��ַ
	int shadow_Inode_Addr = -1;	//�û������ļ�inode��ַ
	int group_Inode_Addr = -1;	//�û����ļ�inode��ַ
	Inode passwd_Inode;		//�û��ļ���inode
	Inode shadow_Inode;		//�û������ļ���inode
	Inode group_Inode;		//�û����ļ�inode
	//ԭ����Ŀ¼
	char bak_Cur_User_Name[110];
	char bak_Cur_User_Name_2[110];
	char bak_Cur_User_Dir_Name[310];
	int bak_Cur_Dir_Addr;
	char bak_Cur_Dir_Name[310];
	char bak_Cur_Group_Name[310];

	Inode cur_dir_inode;	//��ǰĿ¼��inode
	int i,j;
	DirItem dirlist[16];	//��ʱĿ¼

	//�����ֳ����ص���Ŀ¼
	strcpy(bak_Cur_User_Name,Cur_User_Name);
	strcpy(bak_Cur_User_Dir_Name,Cur_User_Dir_Name);
	bak_Cur_Dir_Addr = Cur_Dir_Addr;
	strcpy(bak_Cur_Dir_Name,Cur_Dir_Name);

	//�����û�Ŀ¼
	gotoRoot();
	cd(Cur_Dir_Addr,"home");
	//�����ֳ�
	strcpy( bak_Cur_User_Name_2 , Cur_User_Name);
	strcpy( bak_Cur_Group_Name , Cur_Group_Name);
	//����
	strcpy(Cur_User_Name,username);
	strcpy(Cur_Group_Name,"user");
	if(!mkdir(Cur_Dir_Addr,username)){
		strcpy( Cur_User_Name,bak_Cur_User_Name_2);
		strcpy( Cur_Group_Name,bak_Cur_Group_Name);
		//�ָ��ֳ����ص�ԭ����Ŀ¼
		strcpy(Cur_User_Name,bak_Cur_User_Name);
		strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);

		printf("�û�ע��ʧ��!\n");
		return false;
	}
	//�ָ��ֳ�
	strcpy( Cur_User_Name,bak_Cur_User_Name_2);
	strcpy( Cur_Group_Name,bak_Cur_Group_Name);

	//�ص���Ŀ¼
	gotoRoot();

	//�����û�Ŀ¼
	cd(Cur_Dir_Addr,"etc");

	//�����û�����
	char passwd[100] = {0};
	inPasswd(passwd);	//��������

	//�ҵ�passwd��shadow�ļ�inode��ַ����ȡ����׼�������Ŀ

	//ȡ����ǰĿ¼��inode
	fseek(fr,Cur_Dir_Addr,SEEK_SET);	
	fread(&cur_dir_inode,sizeof(Inode),1,fr);

	//����ȡ�����̿飬����passwd�ļ���inode��ַ����shadow�ļ���inode��ַ
	for(i=0;i<10;i++){
		if(cur_dir_inode.i_dirBlock[i]==-1){
			continue;
		}
		//����ȡ�����̿�
		fseek(fr,cur_dir_inode.i_dirBlock[i],SEEK_SET);	
		fread(&dirlist,sizeof(dirlist),1,fr);

		for(j=0;j<16;j++){	//����Ŀ¼��
			if( strcmp(dirlist[j].itemName,"passwd")==0 ||	//�ҵ�passwd����shadow��Ŀ
				strcmp(dirlist[j].itemName,"shadow")==0 ||
				strcmp(dirlist[j].itemName,"group")==0){
				Inode tmp;	//��ʱinode
				//ȡ��inode���ж��Ƿ����ļ�
				fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	
				fread(&tmp,sizeof(Inode),1,fr);

				if( ((tmp.i_mode>>9)&1)==0 ){	
					//���ļ�
					//�б���passwd�ļ�����shadow�ļ�
					if( strcmp(dirlist[j].itemName,"passwd")==0 ){	
						passwd_Inode_Addr = dirlist[j].inodeAddr;
						passwd_Inode = tmp;
					}
					else if(strcmp(dirlist[j].itemName,"shadow")==0 ){
						shadow_Inode_Addr = dirlist[j].inodeAddr;
						shadow_Inode = tmp;
					}
					else if(strcmp(dirlist[j].itemName,"group")==0 ){
						group_Inode_Addr = dirlist[j].inodeAddr;
						group_Inode = tmp;
					}
				}
			}
		}
		if(passwd_Inode_Addr!=-1 && shadow_Inode_Addr!=-1)	//���ҵ���
			break;
	}

	//����passwd�ļ������Ƿ�����û�username
	char buf[100000];	//���100K���ݴ�passwd���ļ�����
	char buf2[600];		//�ݴ���̿�����
	j = 0;	//���̿�ָ��
	//ȡ��passwd�ļ�����
	for(i=0;i<passwd_Inode.i_size;i++){
		if(i%superblock->s_BLOCK_SIZE==0){	//������
			//���µĴ��̿�
			fseek(fr,passwd_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);	
			fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
			j = 0;
		}
		buf[i] = buf2[j++];
	}
	buf[i] = '\0';

	if(strstr(buf,username)!=NULL){
		//û�ҵ����û�
		printf("�û��Ѵ���\n");

		//�ָ��ֳ����ص�ԭ����Ŀ¼
		strcpy(Cur_User_Name,bak_Cur_User_Name);
		strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);
		return false;
	}
	
	//��������ڣ���passwd�д������û���Ŀ,�޸�group�ļ�
	sprintf(buf+strlen(buf),"%s:x:%d:%d\n",username,nextUID++,1);	//������Ŀ���û������������룺�û�ID���û���ID���û���Ϊ��ͨ�û��飬ֵΪ1 
	passwd_Inode.i_size = strlen(buf);
	writefile(passwd_Inode,passwd_Inode_Addr,buf);	//���޸ĺ��passwdд���ļ���
	
	//ȡ��shadow�ļ�����
	j = 0;
	for(i=0;i<shadow_Inode.i_size;i++){
		if(i%superblock->s_BLOCK_SIZE==0){	//������������̿�
			//���µĴ��̿�
			fseek(fr,shadow_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);	
			fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
			j = 0;
		}
		buf[i] = buf2[j++];
	}
	buf[i] = '\0';

	//����shadow��Ŀ
	sprintf(buf+strlen(buf),"%s:%s\n",username,passwd);	//������Ŀ���û���������
	shadow_Inode.i_size = strlen(buf);
	writefile(shadow_Inode,shadow_Inode_Addr,buf);	//���޸ĺ������д���ļ���


	//ȡ��group�ļ�����
	j = 0;
	for(i=0;i<group_Inode.i_size;i++){
		if(i%superblock->s_BLOCK_SIZE==0){	//������������̿�
			//���µĴ��̿�
			fseek(fr,group_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);	
			fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
			j = 0;
		}
		buf[i] = buf2[j++];
	}
	buf[i] = '\0';

	//����group����ͨ�û��б�
	if(buf[strlen(buf)-2]==':')
		sprintf(buf+strlen(buf)-1,"%s\n",username);	//���������û�
	else 
		sprintf(buf+strlen(buf)-1,",%s\n",username);	//���������û�
	group_Inode.i_size = strlen(buf);
	writefile(group_Inode,group_Inode_Addr,buf);	//���޸ĺ������д���ļ���

	//�ָ��ֳ����ص�ԭ����Ŀ¼
	strcpy(Cur_User_Name,bak_Cur_User_Name);
	strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
	Cur_Dir_Addr = bak_Cur_Dir_Addr;
	strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);

	printf("�û�ע��ɹ�\n");
	return true;
}


bool userdel(char username[])	//�û�ɾ��
{
	if(strcmp(Cur_User_Name,"root")!=0){
		printf("Ȩ�޲���:����ҪrootȨ��\n");
		return false;
	}
	if(strcmp(username,"root")==0){
		printf("�޷�ɾ��root�û�\n");
		return false;
	}
	int passwd_Inode_Addr = -1;	//�û��ļ�inode��ַ
	int shadow_Inode_Addr = -1;	//�û������ļ�inode��ַ
	int group_Inode_Addr = -1;	//�û����ļ�inode��ַ
	Inode passwd_Inode;		//�û��ļ���inode
	Inode shadow_Inode;		//�û������ļ���inode
	Inode group_Inode;		//�û����ļ�inode
	//ԭ����Ŀ¼
	char bak_Cur_User_Name[110];
	char bak_Cur_User_Dir_Name[310];
	int bak_Cur_Dir_Addr;
	char bak_Cur_Dir_Name[310];

	Inode cur_dir_inode;	//��ǰĿ¼��inode
	int i,j;
	DirItem dirlist[16];	//��ʱĿ¼

	//�����ֳ����ص���Ŀ¼
	strcpy(bak_Cur_User_Name,Cur_User_Name);
	strcpy(bak_Cur_User_Dir_Name,Cur_User_Dir_Name);
	bak_Cur_Dir_Addr = Cur_Dir_Addr;
	strcpy(bak_Cur_Dir_Name,Cur_Dir_Name);

	//�ص���Ŀ¼
	gotoRoot();

	//�����û�Ŀ¼
	cd(Cur_Dir_Addr,"etc");

	//�����û�����
	//char passwd[100] = {0};
	//inPasswd(passwd);	//��������

	//�ҵ�passwd��shadow�ļ�inode��ַ����ȡ����׼�������Ŀ

	//ȡ����ǰĿ¼��inode
	fseek(fr,Cur_Dir_Addr,SEEK_SET);	
	fread(&cur_dir_inode,sizeof(Inode),1,fr);

	//����ȡ�����̿飬����passwd�ļ���inode��ַ����shadow�ļ���inode��ַ
	for(i=0;i<10;i++){
		if(cur_dir_inode.i_dirBlock[i]==-1){
			continue;
		}
		//����ȡ�����̿�
		fseek(fr,cur_dir_inode.i_dirBlock[i],SEEK_SET);	
		fread(&dirlist,sizeof(dirlist),1,fr);

		for(j=0;j<16;j++){	//����Ŀ¼��
			if( strcmp(dirlist[j].itemName,"passwd")==0 ||	//�ҵ�passwd����shadow��Ŀ
				strcmp(dirlist[j].itemName,"shadow")==0 ||
				strcmp(dirlist[j].itemName,"group")==0){
				Inode tmp;	//��ʱinode
				//ȡ��inode���ж��Ƿ����ļ�
				fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	
				fread(&tmp,sizeof(Inode),1,fr);

				if( ((tmp.i_mode>>9)&1)==0 ){	
					//���ļ�
					//�б���passwd�ļ�����shadow�ļ�
					if( strcmp(dirlist[j].itemName,"passwd")==0 ){	
						passwd_Inode_Addr = dirlist[j].inodeAddr;
						passwd_Inode = tmp;
					}
					else if(strcmp(dirlist[j].itemName,"shadow")==0 ){
						shadow_Inode_Addr = dirlist[j].inodeAddr;
						shadow_Inode = tmp;
					}
					else if(strcmp(dirlist[j].itemName,"group")==0 ){
						group_Inode_Addr = dirlist[j].inodeAddr;
						group_Inode = tmp;
					}
				}
			}
		}
		if(passwd_Inode_Addr!=-1 && shadow_Inode_Addr!=-1)	//���ҵ���
			break;
	}

	//����passwd�ļ������Ƿ�����û�username
	char buf[100000];	//���100K���ݴ�passwd���ļ�����
	char buf2[600];		//�ݴ���̿�����
	j = 0;	//���̿�ָ��
	//ȡ��passwd�ļ�����
	for(i=0;i<passwd_Inode.i_size;i++){
		if(i%superblock->s_BLOCK_SIZE==0){	//������
			//���µĴ��̿�
			fseek(fr,passwd_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);	
			fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
			j = 0;
		}
		buf[i] = buf2[j++];
	}
	buf[i] = '\0';

	if(strstr(buf,username)==NULL){
		//û�ҵ����û�
		printf("�û�������\n");

		//�ָ��ֳ����ص�ԭ����Ŀ¼
		strcpy(Cur_User_Name,bak_Cur_User_Name);
		strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
		Cur_Dir_Addr = bak_Cur_Dir_Addr;
		strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);
		return false;
	}
	
	//������ڣ���passwd��shadow��group��ɾ�����û�����Ŀ
	//ɾ��passwd��Ŀ
	char *p = strstr(buf,username);
	*p = '\0';
	while((*p)!='\n')	//�ճ��м�Ĳ���
		p++;
	p++;
	strcat(buf,p);	
	passwd_Inode.i_size = strlen(buf);	//�����ļ���С
	writefile(passwd_Inode,passwd_Inode_Addr,buf);	//���޸ĺ��passwdд���ļ���
	
	//ȡ��shadow�ļ�����
	j = 0;
	for(i=0;i<shadow_Inode.i_size;i++){
		if(i%superblock->s_BLOCK_SIZE==0){	//������������̿�
			//���µĴ��̿�
			fseek(fr,shadow_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);	
			fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
			j = 0;
		}
		buf[i] = buf2[j++];
	}
	buf[i] = '\0';

	//ɾ��shadow��Ŀ
	p = strstr(buf,username);
	*p = '\0';
	while((*p)!='\n')	//�ճ��м�Ĳ���
		p++;
	p++;
	strcat(buf,p);	
	shadow_Inode.i_size = strlen(buf);	//�����ļ���С
	writefile(shadow_Inode,shadow_Inode_Addr,buf);	//���޸ĺ������д���ļ���


	//ȡ��group�ļ�����
	j = 0;
	for(i=0;i<group_Inode.i_size;i++){
		if(i%superblock->s_BLOCK_SIZE==0){	//������������̿�
			//���µĴ��̿�
			fseek(fr,group_Inode.i_dirBlock[i/superblock->s_BLOCK_SIZE],SEEK_SET);	
			fread(&buf2,superblock->s_BLOCK_SIZE,1,fr);
			j = 0;
		}
		buf[i] = buf2[j++];
	}
	buf[i] = '\0';

	//����group����ͨ�û��б�
	p = strstr(buf,username);
	*p = '\0';
	while((*p)!='\n' && (*p)!=',')	//�ճ��м�Ĳ���
		p++;
	if((*p)==',')
		p++;
	strcat(buf,p);	
	group_Inode.i_size = strlen(buf);	//�����ļ���С
	writefile(group_Inode,group_Inode_Addr,buf);	//���޸ĺ������д���ļ���

	//�ָ��ֳ����ص�ԭ����Ŀ¼
	strcpy(Cur_User_Name,bak_Cur_User_Name);
	strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
	Cur_Dir_Addr = bak_Cur_Dir_Addr;
	strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);

	//ɾ���û�Ŀ¼	
	Cur_Dir_Addr = Root_Dir_Addr;	//��ǰ�û�Ŀ¼��ַ��Ϊ��Ŀ¼��ַ
	strcpy(Cur_Dir_Name,"/");		//��ǰĿ¼��Ϊ"/"
	cd(Cur_Dir_Addr,"home");
	rmdir(Cur_Dir_Addr,username);

	//�ָ��ֳ����ص�ԭ����Ŀ¼
	strcpy(Cur_User_Name,bak_Cur_User_Name);
	strcpy(Cur_User_Dir_Name,bak_Cur_User_Dir_Name);
	Cur_Dir_Addr = bak_Cur_Dir_Addr;
	strcpy(Cur_Dir_Name,bak_Cur_Dir_Name);

	printf("�û���ɾ��\n");
	return true;
	
}

void chmod(int parinoAddr,char name[],int pmode)	//�޸��ļ���Ŀ¼Ȩ��//�ļ������Լ�pmode
{
	if(strlen(name)>=MAX_NAME_SIZE){
		printf("�������Ŀ¼������\n");
		return ;
	}
	if(strcmp(name,".")==0 || strcmp(name,"..")==0){
		printf("�������\n");
		return ;
	}
	//ȡ�����ļ���Ŀ¼inode
	Inode cur,fileInode;
	fseek(fr,parinoAddr,SEEK_SET);//���ļ�ָ���ƶ�����Ŀ¼��inode
	fread(&cur,sizeof(Inode),1,fr);//��ȡ��Ŀ¼��inode

	//����ȡ�����̿�
	int i = 0,j;
	DirItem dirlist[16] = {0};
	while(i<160){
		if(cur.i_dirBlock[i/16]==-1){
			i+=16;
			continue;
		}
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i/16];
		fseek(fr,parblockAddr,SEEK_SET);	
		fread(&dirlist,sizeof(dirlist),1,fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		for(j=0;j<16;j++){
			if( strcmp(dirlist[j].itemName,name)==0 ){	//�ҵ���Ŀ¼�����ļ�
				//ȡ����Ӧ��inode
				fseek(fr,dirlist[j].inodeAddr,SEEK_SET);//���ļ�ָ���ƶ�����Ŀ¼��inode	
				fread(&fileInode,sizeof(Inode),1,fr);//��ȡ��Ŀ¼��inode
				fflush(fr);
				goto label;
			}
		}
		i++;
	}
label:
	if(i>=160){
		printf("�ļ�������\n");
		return ;
	}
	//�ж��Ƿ��Ǳ��û�������һ������Ա��Ȩ��
	if(strcmp(Cur_User_Name,fileInode.i_uname)!=0 && strcmp(Cur_User_Name,"root")!=0){
		printf("Ȩ�޲���\n");
		return ;
	}

	//��inode��mode���Խ����޸�
	fileInode.i_mode  = (fileInode.i_mode>>9<<9) | pmode;	//�޸�Ȩ��

	//��inodeд�ش���
	fseek(fw,dirlist[j].inodeAddr,SEEK_SET);	//���ļ�ָ���ƶ�����Ŀ¼��inode
	fwrite(&fileInode,sizeof(Inode),1,fw);//��ȡ��Ŀ¼��inode
	fflush(fw);//ˢ�»�����
}

void touch(int parinoAddr,char name[],char buf[])	//touch������ļ��������ַ�
{
	//���ж��ļ��Ƿ��Ѵ��ڡ�������ڣ�������ļ����༭
	if(strlen(name)>=MAX_NAME_SIZE){
		printf("��������ļ�������\n");
		return ;
	}
	//��������ͬ���ļ����еĻ���ʾ�����˳�����û�еĻ�������һ�����ļ�
	DirItem dirlist[16];	//��ʱĿ¼�嵥

	//�������ַȡ��inode
	Inode cur,fileInode;
	fseek(fr,parinoAddr,SEEK_SET);	
	fread(&cur,sizeof(Inode),1,fr);

	//�ж��ļ�ģʽ��6Ϊowner��3Ϊgroup��0Ϊother
	int filemode;
	if(strcmp(Cur_User_Name,cur.i_uname)==0 ) 
		filemode = 6;
	else if(strcmp(Cur_User_Name,cur.i_gname)==0)
		filemode = 3;
	else 
		filemode = 0;
	
	int i = 0,j;
	int dno;
	int fileInodeAddr = -1;	//�ļ���inode��ַ
	while(i<160){
		//160��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
		dno = i/16;	//�ڵڼ���ֱ�ӿ���

		if(cur.i_dirBlock[dno]==-1){
			i+=16;
			continue;
		}
		fseek(fr,cur.i_dirBlock[dno],SEEK_SET);	
		fread(dirlist,sizeof(dirlist),1,fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		for(j=0;j<16;j++){
			if(strcmp(dirlist[j].itemName,name)==0 ){
				//������ȡ��inode���ж��Ƿ����ļ�
				fseek(fr,dirlist[j].inodeAddr,SEEK_SET);	
				fread(&fileInode,sizeof(Inode),1,fr);
				if( ((fileInode.i_mode>>9)&1)==0 ){	//���ļ�����������ʾ�����˳�����
					printf("�ļ��Ѵ���\n");
					return ;
				}
			}
			i++;
		}
	}

	//�ļ������ڣ�����һ�����ļ�
	if( ((cur.i_mode>>filemode>>1)&1)==1){
		//��д�����Դ����ļ�
		buf[0] = '\0';
		create(parinoAddr,name,buf);	//�����ļ�
	}
	else{
		printf("Ȩ�޲��㣺��д��Ȩ��\n");
		return ;
	}
	
}

void help()	//��ʾ���������嵥 
{
	printf("ls - ��ʾ��ǰĿ¼�嵥\n");
	printf("cd - ����Ŀ¼\n");
	printf("mkdir - ����Ŀ¼\n");
	printf("rmdir - ɾ��Ŀ¼\n");
	printf("super - �鿴������\n");
	printf("inode - �鿴inodeλͼ\n");
	printf("block - �鿴blockλͼ\n");
	printf("vi - vi�༭��\n");
	printf("touch - ����һ�����ļ�\n");
	printf("rm - ɾ���ļ�\n");
	printf("cls - ����\n");
	printf("create - �����ļ�\n");
	printf("logout - �û�ע��\n");
	printf("useradd - ����û�\n");
	printf("userdel - ɾ���û�\n");
	printf("chmod - �޸��ļ���Ŀ¼Ȩ��\n");
	printf("help - ��ʾ�����嵥\n");
	printf("exit - �˳�ϵͳ\n");
	return ;
}

void cmd(char str[])	//�������������
{
	char p1[100];
	char p2[100];
	char p3[100];
	char buf[100000];	//���100K
	int tmp = 0;
	int i;
	sscanf(str,"%s",p1);//L�ȶ�ȡstr�ĵ�һ���ַ���
	if(strcmp(p1,"ls")==0){//L�ж��Ƿ���ʾ��ǰĿ¼
		ls(Cur_Dir_Addr);	//��ʾ��ǰĿ¼
	}
	else if(strcmp(p1,"cd")==0){//L�ж��Ƿ����Ŀ¼
		sscanf(str,"%s%s",p1,p2);//L��ȡstr�ĵڶ����ַ���
		cd(Cur_Dir_Addr,p2);	
	}
	else if(strcmp(p1,"mkdir")==0){//L�ж��Ƿ񴴽�Ŀ¼
		sscanf(str,"%s%s",p1,p2);//L��ȡstr�ĵڶ����ַ���
		mkdir(Cur_Dir_Addr,p2);	//L�ڵ�ǰ��Ŀ¼�´���Ŀ¼
	}
	else if(strcmp(p1,"rmdir")==0){//L�ж��Ƿ�ɾ��Ŀ¼
		sscanf(str,"%s%s",p1,p2);
		rmdir(Cur_Dir_Addr,p2);	//L�ڵ�ǰ��Ŀ¼��ɾ��Ŀ¼
	}
	else if(strcmp(p1,"super")==0){//L�ж��Ƿ���ʾ������
		printSuperBlock();//L��ʾ������
	}
	else if(strcmp(p1,"inode")==0){//L�ж��Ƿ���ʾinodeλͼ
		printInodeBitmap();
	}
	else if(strcmp(p1,"block")==0){//L�ж��Ƿ���ʾblockλͼ
		sscanf(str,"%s%s",p1,p2);
		tmp = 0;
		if('0'<=p2[0] && p2[0]<='9'){
			tmp=atoi(p2);
			// for(i=0;p2[i];i++)
			// 	tmp = tmp*10+p2[i]-'0';
			printBlockBitmap(tmp);
		}
		else
			printBlockBitmap();
	}
	else if(strcmp(p1,"vi")==0){	//����һ���ļ�
		sscanf(str,"%s%s",p1,p2);
		vi(Cur_Dir_Addr,p2,buf);	//��ȡ���ݵ�buf
	}
	else if(strcmp(p1,"touch")==0){
		sscanf(str,"%s%s",p1,p2);
		touch (Cur_Dir_Addr,p2,buf);	//��ȡ���ݵ�buf
	}
	else if(strcmp(p1,"rm")==0){	//ɾ��һ���ļ�
		sscanf(str,"%s%s",p1,p2);
		del(Cur_Dir_Addr,p2);
	}
	else if(strcmp(p1,"cls")==0){//L�򵥵���������
		system("cls");
	}
	else if(strcmp(p1,"logout")==0){//L�û�ע��,�˳���½
		logout();
	}
	else if(strcmp(p1,"useradd")==0){//L�����û�
		p2[0] = '\0';
		sscanf(str,"%s%s",p1,p2);
		if(strlen(p2)==0){
			printf("��������\n");
		}
		else{
			useradd(p2);
		}
	}
	else if(strcmp(p1,"userdel")==0){//Lɾ���û�
		p2[0] = '\0';
		sscanf(str,"%s%s",p1,p2);
		if(strlen(p2)==0){
			printf("��������\n");
		}
		else{
			userdel(p2);
		}
	}
	else if(strcmp(p1,"chmod")==0){//L�޸��ļ���Ŀ¼Ȩ��
		p2[0] = '\0';
		p3[0] = '\0';
		sscanf(str,"%s%s%s",p1,p2,p3);
		if(strlen(p2)==0 || strlen(p3)==0){
			printf("��������\n");
		}
		else{
			tmp = 0;
			for(i=0;p3[i];i++)
				tmp = tmp*8+p3[i]-'0';
			chmod(Cur_Dir_Addr,p2,tmp);
		}
	}
	else if(strcmp(p1,"help")==0){//L��ʾ������Ϣ
		help();
	}
	else if(strcmp(p1,"format")==0){//L��ʽ������
		if(strcmp(Cur_User_Name,"root")!=0){
			printf("Ȩ�޲��㣺����ҪrootȨ��\n");
			return ;
		}
		Ready();
		logout();
	}
	else if(strcmp(p1,"exit")==0){
		printf("�˳�MingOS\n");
		exit(0);
	}
	else if(strcmp(p1,"create")==0){
		p2[0] = '\0';
		sscanf(str,"%s%s%s",p1,p2,p3);
		create(Cur_Dir_Addr,p2,p3);
	}
	else{
		printf("��Ǹ��û�и�����\n");
	}
	return ;
}
