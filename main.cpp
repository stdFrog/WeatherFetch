#include "resource.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

struct xmlMemory{
	char* Text;
	size_t Size;
};

size_t WriteCallback(void *pBuffer, size_t Size, size_t NumberOfMembers, void *pArgs) {
	struct xmlMemory *pMem = (struct xmlMemory*)pArgs;

	size_t TotalSize = Size * NumberOfMembers;
	char *pTemp = (char*)realloc(pMem->Text, pMem->Size + TotalSize + 1/* '\0' */);

	if(!pTemp){
		fprintf(stderr, "Allocation Failed: %d\n", GetLastError());
		return 0;
	}

	pMem->Text = pTemp;
	memcpy(&(pMem->Text[pMem->Size]), pBuffer, TotalSize);
	pMem->Size += TotalSize;
	pMem->Text[pMem->Size] = '\0';

	return TotalSize;
}

int main() {
	CURL *Curl = curl_easy_init();

	if(!Curl){
		fprintf(stderr, "Curl Init Failed: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
		return -1;
	}

	struct xmlMemory Mem;
	Mem.Text = (char*)malloc(1);
	Mem.Size = 0;

	curl_easy_setopt(Curl, CURLOPT_URL, API_URL);
	curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(Curl, CURLOPT_WRITEDATA, (void*)&Mem);
	curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 1);

	CURLcode res = curl_easy_perform(Curl);
	if(res != CURLE_OK){
		fprintf(stderr, "Perform Failed: %s\n", curl_easy_strerror(res));
		free(Mem.Text);
		curl_easy_cleanup(Curl);
		return -1;
	}

	xmlInitParser();
	xmlDoc *doc = xmlReadMemory(Mem.Text, Mem.Size, NULL, NULL, 0);

	if(!doc){
		fprintf(stderr, "xmlMemory Read Failed\n");
		free(Mem.Text);
		xmlCleanupParser();
		curl_easy_cleanup(Curl);
	}

	xmlNodePtr Root = xmlDocGetRootElement(doc);
	xmlNodePtr Body = Root->children;

	while(Body){
		if(Body->type == XML_ELEMENT_NODE && xmlStrcmp(Body->name, (const xmlChar*)"body") == 0){
			xmlNode *Items = Body->children;

			while(Items){
				if(Items->type == XML_ELEMENT_NODE && xmlStrcmp(Items->name, (const xmlChar*)"items") == 0){
					xmlNode *Item = Items->children;

					while(Item){
						if(Item->type == XML_ELEMENT_NODE && xmlStrcmp(Item->name, (const xmlChar*)"item") == 0){
							int nx = -1,
								ny = -1;

							double Latitude, Longitude;

							xmlNode *Data = Item->children;
							while(Data){
								if(Data->type == XML_ELEMENT_NODE){
									if(xmlStrcmp(Data->name, (const xmlChar*)"nx") == 0){
										nx = atoi((char*)xmlNodeGetContent(Data));
									}
									if(xmlStrcmp(Data->name, (const xmlChar*)"ny") == 0){
										ny = atoi((char*)xmlNodeGetContent(Data));
									}
									printf("%s: %s\n", Data->name, xmlNodeGetContent(Data));
								}

								Data = Data->next;
							}
							printf("----------------------\n");
						}
						Item = Item->next;
					}
				}
				Items = Items->next;
			}
		}
		Body = Body->next;
	}

	free(Mem.Text);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	curl_easy_cleanup(Curl);
	return 0;
}
