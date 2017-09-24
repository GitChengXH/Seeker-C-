#include "stdafx.h"
#include <string>          
#include <iostream>  
#include <fstream> 
//C++ 标准模板库
#include <vector>           
#include "winsock2.h"  
#include <time.h>  
#include <queue>  
#include <hash_set>  
#pragma comment(lib, "ws2_32.lib")   
#define DEFAULT_PAGE_BUF_SIZE  1048576  

using namespace std;

//新建队列存放URL
queue<string> hrefUrl;

//新建哈希集合
hash_set<string> visitedUrl;
hash_set<string> visitedImg;

//定义全局变量
int depth = 0;
int g_ImgCnt = 1;

//解析URL，解析出主机名，资源名  
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


//使用Get请求，得到响应  
bool GetHttpResponse(const string & url, char * &response, int &bytesRead){
	string host, resource;
	if (!ParseURL(url, host, resource)){
		cout << "Can not parse the url" << endl;
		return false;
	}

	//建立socket  
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

	//建立服务器地址  
	SOCKADDR_IN sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);
	//char addr[5];  
	//memcpy( addr, hp->h_addr, 4 );  
	//sa.sin_addr.s_addr = inet_addr(hp->h_addr);  
	memcpy(&sa.sin_addr, hp->h_addr, 4);

	//建立连接  
	if (0 != connect(sock, (SOCKADDR*)&sa, sizeof(sa))){
		cout << "Can not connect: " << url << endl;
		closesocket(sock);
		return false;
	};

	//准备发送数据  
	string request = "GET " + resource + " HTTP/1.1\r\nHost:" + host + "\r\nConnection:Close\r\n\r\n";

	//发送数据  
	if (SOCKET_ERROR == send(sock, request.c_str(), request.size(), 0)){
		cout << "send error" << endl;
		closesocket(sock);
		return false;
	}

	//接收数据  
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
			pageBuf = (char*)realloc(pageBuf, m_nContentLength);            //重新分配内存  
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


//提取所有的URL以及图片URL，并保存到外部文件  
void HTMLParse(string & htmlResponse, vector<string> & imgurls, const string & host){
	//找所有连接，加入queue中  
	//新建指针文件指向网页获取的代码c_str将string转成char
	const char *p = htmlResponse.c_str();
	char *tag = "href=\"";      // “\”为非转义符
	//判断tag是否在p中，如果有返回tag第一次出现的地址
	//网页的HTML源代码中网页连接<li><a href = "http://g.360.cn">游戏< / a>< / li>
	const char *pos = strstr(p, tag);
	//新建一个输出文件流
	ofstream ofile("url.txt", ios::app);
	while (pos)
	{
		//指针向后偏移href = "个长度，找到网站的网址
		pos += strlen(tag);
		//找到网页连接的起点
		const char * nextQ = strstr(pos, "\"");
		if (nextQ){
			char * url = new char[nextQ - pos + 1];
			//char url[100]; //固定大小的会发生缓冲区溢出的危险  
			//匹配上面结果中以"开头的字符，并将字符存放在url中
			sscanf(pos, "%[^\"]", url);
			string surl = url;  // 转换成string类型，可以自动释放内存 
			//如果当前的网页地址为，哈希集合中的最后一个元素则将当前的元素插入
			if (visitedUrl.find(surl) == visitedUrl.end())
			{
				visitedUrl.insert(surl);
				//将找到的网址写入到输出文件 
				ofile << surl << endl;
				hrefUrl.push(surl);
			}

			//pos中存放的是网页连接开始数据
			pos = strstr(pos, tag);
			delete[] url;  // 释放掉申请的内存  
		}
	}

	ofile << endl << endl;
	ofile.close();

	//图片的格式以<img 开头
	tag = "<img ";

	//P中存放的是所有下载来的HTML文件的源码
	const char* att1 = "src=\"";
	const char* att2 = "lazy-src=\"";
	const char *pos0 = strstr(p, tag);


	while (pos0){
		//指针后移
		pos0 += strlen(tag);
		//pos2为lazy-src后面的程序
		const char* pos2 = strstr(pos0, att2);
		//如果pos2为空或者
		if (!pos2 || pos2 > strstr(pos0, ">")) 
		{
			//如果找不到lazy-src开头的字符串就寻找src开头的字符
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

			//得到图片的地址路径，并写入到输出文件
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


//把URL转化为文件名  
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

//下载图片到img文件夹  
void DownLoadImg(vector<string> & imgurls, const string &url){

	//生成保存该url下图片的文件夹  
	string foldname = ToFileName(url);
	foldname = "G:\\C# C++自建工程目录\\【C++ learning】\\Seeker\\Seeker\\img\\" + foldname;
	if (!CreateDirectory(foldname.c_str(), NULL))
		cout << "Can not create directory:" << foldname << endl;
	char *image;
	int byteRead;

	for (int i = 0; i < imgurls.size(); i++)
	{
		//判断是否为图片，bmp，jgp，jpeg，gif   
		//将字符串拼接起来，保存到string
		string str = imgurls[i];
		//找到文件的后缀名
		int pos = str.find_last_of(".");
		//string::npos定位到字符最后位置
		if (pos == string::npos)
			continue;
		else
		{
			//字符串截取函数，从pos+1,往后截取str.size() - pos - 1位，根据截取得到的后缀名判断文件的类型
			string ext = str.substr(pos + 1, str.size() - pos - 1);
			if (ext != "bmp"&& ext != "jpg" && ext != "jpeg"&& ext != "gif"&&ext != "png")
			continue;  
		}

		//下载其中的内容 ，下载的图片保存在image中 
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
			//释放指针内存
			free(image);

		}
	}
}


//广度遍历  传进来待遍历的图片的网址
void BFS(const string & url)
{
	//用来存放网页请求的结果
	char * response;
	int bytes;
	// 获取网页的相应，放入response中。  
	if (!GetHttpResponse(url, response, bytes))
	{
		cout << "The url is wrong! ignore." << endl;
		return;
	}

	string httpResponse = response;

	//清空指针，数据存放在httpResponse字符串中
	free(response);

	string filename = ToFileName(url);
	ofstream ofile("G:\\C# C++自建工程目录\\【C++ learning】\\Seeker\\Seeker\\html\\" + filename);
	if (ofile.is_open())
	{
		// 保存该网页的文本内容  
		ofile << httpResponse << endl;
		ofile.close();
	}

	//新建存放图片的容器
	vector<string> imgurls;

	//解析该网页的所有图片链接，放入imgurls里面  
	HTMLParse(httpResponse, imgurls, url);

	//下载所有的图片资源  
	DownLoadImg(imgurls, url);
}


//主函数
void main()
{
	//初始化socket，用于tcp网络连接 
	//新建网络连接对象
	//WSADATA 是一种分子结构数据，用来存储WSAStartup函数调用返回的Windows Sockets数据
	WSADATA wsaData;
	//返回值为0表示成功
	//MAKEWORD(2.2),使用socket版本2.2
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return;
	}

	// 创建文件夹，保存图片和网页文本文件  
	CreateDirectory("G:\\C# C++自建工程目录\\【C++ learning】\\Seeker\\Seeker\\img", 0);
	CreateDirectory("G:\\C# C++自建工程目录\\【C++ learning】\\Seeker\\Seeker\\html", 0);

	// 遍历的起始地址  ，设置
	string urlStart = "http://hao.360.cn/meinvdaohang.html";

	// 使用广度遍历  
	// 提取网页中的超链接放入hrefUrl中，提取图片链接，下载图片。  
	BFS(urlStart);

	// 访问过的网址保存起来  
	visitedUrl.insert(urlStart);

	//在死循环中一直遍历URLLIST中的网址
	while (hrefUrl.size() != 0)
	{
		string url = hrefUrl.front();  // 从队列的最开始取出一个网址  
		cout << url << endl;
		BFS(url);                      // 遍历提取出来的那个网页，找它里面的超链接网页放入hrefUrl，下载它里面的文本，图片  
		hrefUrl.pop();                 // 遍历完之后，删除这个网址  
	}

	//卸载winsock网络通讯的DLL
	WSACleanup();

	return;

}
