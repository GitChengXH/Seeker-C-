#include "stdafx.h"
#include <string>          
#include <iostream>  
#include <fstream> 
//C++ ��׼ģ���
#include <vector>           
#include "winsock2.h"  
#include <time.h>  
#include <queue>  
#include <hash_set>  
#pragma comment(lib, "ws2_32.lib")   
#define DEFAULT_PAGE_BUF_SIZE  1048576  

using namespace std;

//�½����д��URL
queue<string> hrefUrl;

//�½���ϣ����
hash_set<string> visitedUrl;
hash_set<string> visitedImg;

//����ȫ�ֱ���
int depth = 0;
int g_ImgCnt = 1;

//����URL������������������Դ��  
bool ParseURL(const string & url, string & host, string & resource)
{
	if (strlen(url.c_str()) > 2000) {
		return false;
	}
	const char * pos = strstr(url.c_str(), "http://");
	if (pos == NULL) pos = url.c_str();
	else pos += strlen("http://");
	if (strstr(pos, "/") == 0)
		return false;
	char pHost[100];
	char pResource[2000];
	sscanf(pos, "%[^/]%s", pHost, pResource);
	host = pHost;
	resource = pResource;
	return true;
}


//ʹ��Get���󣬵õ���Ӧ  
bool GetHttpResponse(const string & url, char * &response, int &bytesRead){
	string host, resource;
	if (!ParseURL(url, host, resource)){
		cout << "Can not parse the url" << endl;
		return false;
	}

	//����socket  
	struct hostent * hp = gethostbyname(host.c_str());
	if (hp == NULL){
		cout << "Can not find host address" << endl;
		return false;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == -1 || sock == -2){
		cout << "Can not create sock." << endl;
		return false;
	}

	//������������ַ  
	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);
	//char addr[5];  
	//memcpy( addr, hp->h_addr, 4 );  
	//sa.sin_addr.s_addr = inet_addr(hp->h_addr);  
	memcpy(&sa.sin_addr, hp->h_addr, 4);

	//��������  
	if (0 != connect(sock, (SOCKADDR*)&sa, sizeof(sa))){
		cout << "Can not connect: " << url << endl;
		closesocket(sock);
		return false;
	};

	//׼����������  
	string request = "GET " + resource + " HTTP/1.1\r\nHost:" + host + "\r\nConnection:Close\r\n\r\n";

	//��������  
	if (SOCKET_ERROR == send(sock, request.c_str(), request.size(), 0)){
		cout << "send error" << endl;
		closesocket(sock);
		return false;
	}

	//��������  
	int m_nContentLength = DEFAULT_PAGE_BUF_SIZE;
	char *pageBuf = (char *)malloc(m_nContentLength);
	memset(pageBuf, 0, m_nContentLength);

	bytesRead = 0;
	int ret = 1;
	cout << "Read: ";
	while (ret > 0){
		ret = recv(sock, pageBuf + bytesRead, m_nContentLength - bytesRead, 0);

		if (ret > 0)
		{
			bytesRead += ret;
		}

		if (m_nContentLength - bytesRead < 100){
			cout << "\nRealloc memorry" << endl;
			m_nContentLength *= 2;
			pageBuf = (char*)realloc(pageBuf, m_nContentLength);            //���·����ڴ�  
		}
		cout << ret << " ";
	}
	cout << endl;

	pageBuf[bytesRead] = '\0';
	response = pageBuf;
	closesocket(sock);
	return true;
	//cout<< response <<endl;  
}


//��ȡ���е�URL�Լ�ͼƬURL�������浽�ⲿ�ļ�  
void HTMLParse(string & htmlResponse, vector<string> & imgurls, const string & host){
	//���������ӣ�����queue��  
	//�½�ָ���ļ�ָ����ҳ��ȡ�Ĵ���c_str��stringת��char
	const char *p = htmlResponse.c_str();
	char *tag = "href=\"";      // ��\��Ϊ��ת���
	//�ж�tag�Ƿ���p�У�����з���tag��һ�γ��ֵĵ�ַ
	//��ҳ��HTMLԴ��������ҳ����<li><a href = "http://g.360.cn">��Ϸ< / a>< / li>
	const char *pos = strstr(p, tag);
	//�½�һ������ļ���
	ofstream ofile("url.txt", ios::app);
	while (pos)
	{
		//ָ�����ƫ��href = "�����ȣ��ҵ���վ����ַ
		pos += strlen(tag);
		//�ҵ���ҳ���ӵ����
		const char * nextQ = strstr(pos, "\"");
		if (nextQ){
			char * url = new char[nextQ - pos + 1];
			//char url[100]; //�̶���С�Ļᷢ�������������Σ��  
			//ƥ������������"��ͷ���ַ��������ַ������url��
			sscanf(pos, "%[^\"]", url);
			string surl = url;  // ת����string���ͣ������Զ��ͷ��ڴ� 
			//�����ǰ����ҳ��ַΪ����ϣ�����е����һ��Ԫ���򽫵�ǰ��Ԫ�ز���
			if (visitedUrl.find(surl) == visitedUrl.end())
			{
				visitedUrl.insert(surl);
				//���ҵ�����ַд�뵽����ļ� 
				ofile << surl << endl;
				hrefUrl.push(surl);
			}

			//pos�д�ŵ�����ҳ���ӿ�ʼ����
			pos = strstr(pos, tag);
			delete[] url;  // �ͷŵ�������ڴ�  
		}
	}

	ofile << endl << endl;
	ofile.close();

	//ͼƬ�ĸ�ʽ��<img ��ͷ
	tag = "<img ";

	//P�д�ŵ���������������HTML�ļ���Դ��
	const char* att1 = "src=\"";
	const char* att2 = "lazy-src=\"";
	const char *pos0 = strstr(p, tag);


	while (pos0){
		//ָ�����
		pos0 += strlen(tag);
		//pos2Ϊlazy-src����ĳ���
		const char* pos2 = strstr(pos0, att2);
		//���pos2Ϊ�ջ���
		if (!pos2 || pos2 > strstr(pos0, ">")) 
		{
			//����Ҳ���lazy-src��ͷ���ַ�����Ѱ��src��ͷ���ַ�
			pos = strstr(pos0, att1);
			if (!pos) {
				pos0 = strstr(att1, tag);
				continue;
			}
			else 
			{
				pos = pos + strlen(att1);
			}
		}
		else 
		{
			pos = pos2 + strlen(att2);
		}


		const char * nextQ = strstr(pos, "\"");
		if (nextQ){
			char * url = new char[nextQ - pos + 1];
			sscanf(pos, "%[^\"]", url);
			cout << url << endl;

			//�õ�ͼƬ�ĵ�ַ·������д�뵽����ļ�
			string imgUrl = url;
			if (visitedImg.find(imgUrl) == visitedImg.end()){
				visitedImg.insert(imgUrl);
				imgurls.push_back(imgUrl);
			}
			pos0 = strstr(pos0, tag);
			delete[] url;
		}
	}

	cout << "end of Parse this html" << endl;
}


//��URLת��Ϊ�ļ���  
string ToFileName(const string &url){
	string fileName;
	fileName.resize(url.size());
	int k = 0;
	for (int i = 0; i < (int)url.size(); i++){
		char ch = url[i];
		if (ch != '\\'&&ch != '/'&&ch != ':'&&ch != '*'&&ch != '?'&&ch != '"'&&ch != '<'&&ch != '>'&&ch != '|')
			fileName[k++] = ch;
	}
	return fileName.substr(0, k) + ".txt";
}

//����ͼƬ��img�ļ���  
void DownLoadImg(vector<string> & imgurls, const string &url){

	//���ɱ����url��ͼƬ���ļ���  
	string foldname = ToFileName(url);
	foldname = "G:\\C# C++�Խ�����Ŀ¼\\��C++ learning��\\Seeker\\Seeker\\img\\" + foldname;
	if (!CreateDirectory(foldname.c_str(), NULL))
		cout << "Can not create directory:" << foldname << endl;
	char *image;
	int byteRead;

	for (int i = 0; i < imgurls.size(); i++)
	{
		//�ж��Ƿ�ΪͼƬ��bmp��jgp��jpeg��gif   
		//���ַ���ƴ�����������浽string
		string str = imgurls[i];
		//�ҵ��ļ��ĺ�׺��
		int pos = str.find_last_of(".");
		//string::npos��λ���ַ����λ��
		if (pos == string::npos)
			continue;
		else
		{
			//�ַ�����ȡ��������pos+1,�����ȡstr.size() - pos - 1λ�����ݽ�ȡ�õ��ĺ�׺���ж��ļ�������
			string ext = str.substr(pos + 1, str.size() - pos - 1);
			if (ext != "bmp"&& ext != "jpg" && ext != "jpeg"&& ext != "gif"&&ext != "png")
			continue;  
		}

		//�������е����� �����ص�ͼƬ������image�� 
		if (GetHttpResponse(imgurls[i], image, byteRead))
		{
			if (strlen(image) == 0)
			{
				continue;
			}
			const char *p = image;
			const char * pos = strstr(p, "\r\n\r\n") + strlen("\r\n\r\n");
			int index = imgurls[i].find_last_of("/");
			if (index != string::npos)
			{
				string imgname = imgurls[i].substr(index, imgurls[i].size());
				ofstream ofile(foldname + imgname, ios::binary);
				if (!ofile.is_open())
					continue;
				cout << g_ImgCnt++ << foldname + imgname << endl;
				ofile.write(pos, byteRead - (pos - p));
				ofile.close();
			}
			//�ͷ�ָ���ڴ�
			free(image);

		}
	}
}


//��ȱ���  ��������������ͼƬ����ַ
void BFS(const string & url)
{
	//���������ҳ����Ľ��
	char * response;
	int bytes;
	// ��ȡ��ҳ����Ӧ������response�С�  
	if (!GetHttpResponse(url, response, bytes))
	{
		cout << "The url is wrong! ignore." << endl;
		return;
	}

	string httpResponse = response;

	//���ָ�룬���ݴ����httpResponse�ַ�����
	free(response);

	string filename = ToFileName(url);
	ofstream ofile("G:\\C# C++�Խ�����Ŀ¼\\��C++ learning��\\Seeker\\Seeker\\html\\" + filename);
	if (ofile.is_open())
	{
		// �������ҳ���ı�����  
		ofile << httpResponse << endl;
		ofile.close();
	}

	//�½����ͼƬ������
	vector<string> imgurls;

	//��������ҳ������ͼƬ���ӣ�����imgurls����  
	HTMLParse(httpResponse, imgurls, url);

	//�������е�ͼƬ��Դ  
	DownLoadImg(imgurls, url);
}


//������
void main()
{
	//��ʼ��socket������tcp�������� 
	//�½��������Ӷ���
	//WSADATA ��һ�ַ��ӽṹ���ݣ������洢WSAStartup�������÷��ص�Windows Sockets����
	WSADATA wsaData;
	//����ֵΪ0��ʾ�ɹ�
	//MAKEWORD(2.2),ʹ��socket�汾2.2
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return;
	}

	// �����ļ��У�����ͼƬ����ҳ�ı��ļ�  
	CreateDirectory("G:\\C# C++�Խ�����Ŀ¼\\��C++ learning��\\Seeker\\Seeker\\img", 0);
	CreateDirectory("G:\\C# C++�Խ�����Ŀ¼\\��C++ learning��\\Seeker\\Seeker\\html", 0);

	// ��������ʼ��ַ  ������
	string urlStart = "http://hao.360.cn/meinvdaohang.html";

	// ʹ�ù�ȱ���  
	// ��ȡ��ҳ�еĳ����ӷ���hrefUrl�У���ȡͼƬ���ӣ�����ͼƬ��  
	BFS(urlStart);

	// ���ʹ�����ַ��������  
	visitedUrl.insert(urlStart);

	//����ѭ����һֱ����URLLIST�е���ַ
	while (hrefUrl.size() != 0)
	{
		string url = hrefUrl.front();  // �Ӷ��е��ʼȡ��һ����ַ  
		cout << url << endl;
		BFS(url);                      // ������ȡ�������Ǹ���ҳ����������ĳ�������ҳ����hrefUrl��������������ı���ͼƬ  
		hrefUrl.pop();                 // ������֮��ɾ�������ַ  
	}

	//ж��winsock����ͨѶ��DLL
	WSACleanup();

	return;

}
