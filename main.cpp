#include "resource.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <cjson/cJSON.h>

struct CodeTable{
	const char* Category;
	const char* Mean;
	const char* Unit;
};

struct xmlMemory{
	char* Text;
	size_t Size;
};

struct jsonMemory{
	char* Text;
	size_t Size;
};

const struct CodeTable CategoryTable[] = {
	{ "POP", "Probability of Precipitation", "(%)" },
	{ "PTY", "Precipitation Type", "" }, 
	{ "PCP", "Hourly Precipitation Amount", "(mm)" },
	{ "REH", "Humidity", "(%)" },
	{ "SNO", "Hourly New Snowfall", "(cm)" },
	{ "SKY", "Sky Condition", "" },
	{ "TMP", "Hourly Temperature", "(Celcius)" },
	{ "TMN", "Daily Minimum Temperature", "(Celcius)" },
	{ "TMX", "Daily Maximum Temperature", "(Celcius)" },
	{ "UUU", "Wind Speed (East-West Component)", "(m/s)" },
	{ "VVV", "Wind Speed (North-South Component)", "(m/s)" },
	{ "WAV", "Wave Height", "(M)" },
	{ "VEC", "Wind Direction", "(deg)"},
	{ "WSD", "Wind Speed", "(m/s)" }
};

#define RadiusOfTheEarth	6371.00877			// 지도 반경 - 평균 반경 (km)
#define GridSpacing			5.0					// 격자 간격 (km)
#define StandardLatitude1	30.0				// 표준위도 1(육십분법)
#define StandardLatitude2	60.0				// 표준위도 2
#define OriginLatitude		38.0				// 기준점 위도(서울 - 기상청, 남북)
#define OriginLongitude		126.0				// 기준점 경도(동서)
#define GridOriginX			210/GridSpacing		// 격자 기준점 X
#define GridOriginY			675/GridSpacing		// 격자 기준점 Y

void GridToLatLon(int NX, int NY, double *Latitude, double *Longitude){
	double PI = atan(1) * 4.0;
	double DegToRad = PI / 180.0;
	double RE = RadiusOfTheEarth / GridSpacing;
	double Slat1Rad = StandardLatitude1 * DegToRad;
	double Slat2Rad = StandardLatitude2 * DegToRad;
	double OlonRad = OriginLongitude * DegToRad;
	double OlatRad = OriginLatitude * DegToRad;

	// 투영 변환(구면 좌표 -> 평면)
	// 투영된 좌표를 위도/경도로 변환할 때 기준이 되는값을 미리 계산한다.

	// sn(투영 계수)은 격자 좌표 및 지리 좌표 비율 결정에 사용할 변수
	double sn = tan(PI * 0.25 + Slat2Rad * 0.5) / tan(PI * 0.25 + Slat1Rad * 0.5);	// 스케일 적용
	sn = log(cos(Slat1Rad) / cos(Slat2Rad)) / log(sn);	// 위도 비율 계산 후 투영 계수 구해옴

	// sf(스케일 팩터) 계산
	double sf = tan(PI * 0.25 + Slat1Rad * 0.5);		// 표준 위도1 기준으로 변환
	sf = pow(sf, sn) * cos(Slat1Rad) / sn;				// 투영 계수 적용, 거리비 조정(왜곡 최소화)

	// ro(기준 반경) 계산, 즉 기준점에서의 반경 결정
	// 반경에 스케일 팩터 적용, 기준점(OriginLatitude)에 대한 변환 계산
	double ro = RE * sf / pow(tan(PI * 0.25 + OlatRad * 0.5), sn);		

	// 상대적 위치 계산
	double xn = NX - GridOriginX;
	double yn = ro - (NY - GridOriginY);

	// 극좌표 변환
	// ra(거리) 및 theta(방향각) 계산
	double ra = sqrt(xn * xn + yn * yn);
	double Theta = atan2(xn, yn);

	// 위도 및 경도 변환
	*Latitude = 2.0 * atan(pow((RE * sf / ra), (1.0 / sn))) - PI * 0.5;
	*Longitude = OlonRad + Theta;
	*Latitude *= 180.0 / PI;
	*Longitude *= 180.0 / PI;
}

void LatLonToGrid(double Latitude, double Longitude, int *NX, int *NY){
	double PI = atan(1) * 4.0;
	double DegToRad = PI / 180.0;
	double RE = RadiusOfTheEarth / GridSpacing;
	double Slat1Rad = StandardLatitude1 * DegToRad;
	double Slat2Rad = StandardLatitude2 * DegToRad;
	double OlonRad = OriginLongitude * DegToRad;
	double OlatRad = OriginLatitude * DegToRad;

	// 투영 변환(구면 좌표 -> 평면)
	// 투영된 좌표를 위도/경도로 변환할 때 기준이 되는값을 미리 계산한다.

	// sn(투영 계수)은 격자 좌표 및 지리 좌표 비율 결정에 사용할 변수
	double sn = tan(PI * 0.25 + Slat2Rad * 0.5) / tan(PI * 0.25 + Slat1Rad * 0.5);	// 스케일 적용
	sn = log(cos(Slat1Rad) / cos(Slat2Rad)) / log(sn);	// 위도 비율 계산 후 투영 계수 구해옴

	// sf(스케일 팩터) 계산
	double sf = tan(PI * 0.25 + Slat1Rad * 0.5);		// 표준 위도1 기준으로 변환
	sf = pow(sf, sn) * cos(Slat1Rad) / sn;				// 투영 계수 적용, 거리비 조정(왜곡 최소화)

	// ro(기준 반경) 계산, 즉 기준점에서의 반경 결정
	// 반경에 스케일 팩터 적용, 기준점(OriginLatitude)에 대한 변환 계산
	double ro = RE * sf / pow(tan(PI * 0.25 + OlatRad * 0.5), sn);		

	// 위경도 -> 격자 좌표(Lambert projection)
	double ra = RE * sf / pow(tan(PI * 0.25 + Latitude * DegToRad * 0.5), sn);

	// 경도 차이 계산 후 범위 조정
	double Theta = Longitude * DegToRad - OlonRad;
	if(Theta > PI){ Theta -= 2.0 * PI; }
	if(Theta < (-PI)){ Theta += 2.0 * PI; }

	// 투영 좌표에 회전 변환 적용
	Theta *= sn;

	// 최종 KMA 격자 좌표 계산
	*NX = (int)(ra * sin(Theta) + GridOriginX + 0.5);
	*NY = (int)(ro - ra * cos(Theta) + GridOriginY + 0.5);
}

int CurrentTime(){
	SYSTEMTIME st;
	GetLocalTime(&st);
	return st.wHour;
}

char* GetDate(){
	char *buf = NULL;
	SYSTEMTIME st;
	GetLocalTime(&st);
	buf = (char*)malloc(9);
	sprintf(buf, "%04d%02d%02d", st.wYear, st.wMonth, st.wDay);
	return buf;
}

int Aprx(int Time){
	int t, c;

	for(int i=0; i<sizeof(BASETIME_TABLE) / sizeof(BASETIME_TABLE[0]); i++){
		t = BASETIME_TABLE[i] + 2;
		c = Time;

		if(t > 23){
			t = 24 - t;
			c = Time - 24;
		}

		if(t >= c){
			return i;
		}
	}

	return 2; /* default */
}


int FindCity(char* Input, char **CityCode){
	for(int i = 0; i<sizeof(CityList)/sizeof(CityList[0]); i++){
		if(strcmp(CityList[i].Name, Input) == 0){
			*CityCode = (char*)CityList[i].Code;
			return 1;
		}
	}

	printf("입력한 도시를 찾을 수 없습니다: %s\n현재는 서울/인천/경기 지역만 조회 가능합니다.\n", Input);
	return 0;
}

size_t GEOWriteCallback(void *pBuffer, size_t Size, size_t NumberOfMembers, void *pArgs){
	size_t TotalSize = Size * NumberOfMembers;
	struct jsonMemory *pMem = (struct jsonMemory *)pArgs;

	pMem->Text = (char*)realloc(pMem->Text, pMem->Size + TotalSize + 1);
	if(pMem->Text == NULL) { return 0; }

	memcpy(&(pMem->Text[pMem->Size]), pBuffer, TotalSize);
	pMem->Size += TotalSize;
	pMem->Text[pMem->Size] = 0;

	return TotalSize;
}

void GetLocationName(double Latitude, double Longitude, char* buf){
	CURL *Curl;
	CURLcode Result;

	char URL[0x200];
	struct jsonMemory Chunk = { NULL, 0 };

	snprintf(URL, sizeof(URL), GEOCODING_URL, Latitude, Longitude, GEOCODING_API_KEY);
	URL[strlen(URL)] = 0;

	Curl = curl_easy_init();
	if(Curl == NULL){
		fprintf(stderr, "Curl Init Failed: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
		return;
	}

	Chunk.Text = (char*)malloc(1);

	curl_easy_setopt(Curl, CURLOPT_URL, URL);
	curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, GEOWriteCallback);
	curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Chunk);

	Result = curl_easy_perform(Curl);
	if(Result == CURLE_OK){
		cJSON *Root = cJSON_Parse(Chunk.Text);
		if(Root == NULL){ 
			fprintf(stderr, "Parse Failed: %s", cJSON_GetErrorPtr());
			return;
		}

		cJSON *ret = cJSON_GetObjectItem(Root, "results");
		if(cJSON_IsArray(ret) && cJSON_GetArraySize(ret) > 0){
			cJSON *FirstResult = cJSON_GetArrayItem(ret, 0);
			cJSON *FormattedAddress = cJSON_GetObjectItem(FirstResult, "formatted_address");

			if(cJSON_IsString(FormattedAddress)){
				sprintf(buf, "%s", FormattedAddress->valuestring);
			}
		}
		cJSON_Delete(Root);
	}

	free(Chunk.Text);
	curl_easy_cleanup(Curl);
}

size_t KMAWriteCallbackFZ(void *pBuffer, size_t Size, size_t NumberOfMembers, void *pArgs){
	size_t TotalSize = Size * NumberOfMembers;
	struct jsonMemory *pMem = (struct jsonMemory *)pArgs;

	pMem->Text = (char*)realloc(pMem->Text, pMem->Size + TotalSize + 1);
	if(pMem->Text == NULL) { return 0; }

	memcpy(&(pMem->Text[pMem->Size]), pBuffer, TotalSize);
	pMem->Size += TotalSize;
	pMem->Text[pMem->Size] = 0;

	return TotalSize;
}

void GetForecastZone(char* CityCode, double *Latitude, double *Longitude){
	CURL *Curl;
	CURLcode Result;

	char URL[0x200];
	struct jsonMemory Chunk = { NULL, 0 };

	snprintf(URL, sizeof(URL), KMA_URL_FORECAST_ZONE, KMA_API_KEY_FORECAST_ZONE, NUMOFROWS2, PAGENO1, DATATYPE2, CityCode);
	URL[strlen(URL)] = 0;

	Curl = curl_easy_init();
	if(Curl == NULL){ 
		fprintf(stderr, "Curl Init Failed: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
		return;
	}

	Chunk.Text = (char*)malloc(1);

	curl_easy_setopt(Curl, CURLOPT_URL, URL);
	curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, KMAWriteCallbackFZ);
	curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Chunk);

	Result = curl_easy_perform(Curl);
	if(Result == CURLE_OK){
		cJSON *Root = cJSON_Parse(Chunk.Text);
		if(Root == NULL){
			fprintf(stderr, "JSON Parse Failed: %s\n", cJSON_GetErrorPtr());
			return;
		}

		cJSON *Body = cJSON_GetObjectItem(Root, "response");
		if(Body == NULL){
			fprintf(stderr, "JSON GetObjectItem Failed: Missing Response Element in JSON\n");
			cJSON_Delete(Root);
			return;
		}

		cJSON *Items = cJSON_GetObjectItem(Body, "body");
		if(Items == NULL){
			fprintf(stderr, "JSON GetObjectItem Failed: Missing Body Element in JSON\n");
			cJSON_Delete(Root);
			return;
		}

		cJSON *ItemArray = cJSON_GetObjectItem(Items, "items");
		if(ItemArray == NULL || !cJSON_IsObject(ItemArray)){
			fprintf(stderr, "JSON GetObjectItem Failed: Missing Items Element in JSON\n");
			cJSON_Delete(Root);
			return;
		}

		cJSON *Item = cJSON_GetObjectItem(ItemArray, "item");
		if(Item == NULL || !cJSON_IsArray(Item)){
			fprintf(stderr, "JSON GetObjectItem Failed: Missing ItemArray Element in JSON\n");
			cJSON_Delete(Root);
			return;
		}

		cJSON *FirstItem = cJSON_GetArrayItem(Item, 0);
		if(FirstItem){
			cJSON *LatitudeJSON = cJSON_GetObjectItem(FirstItem, "lat");
			cJSON *LongitudeJSON = cJSON_GetObjectItem(FirstItem, "lon");

			if (cJSON_IsNumber(LatitudeJSON) && cJSON_IsNumber(LongitudeJSON)) {
				*Latitude = LatitudeJSON->valuedouble;
				*Longitude = LongitudeJSON->valuedouble;
			}
		}
	}

	free(Chunk.Text);
	curl_easy_cleanup(Curl);
}

size_t KMAWriteCallback(void *pBuffer, size_t Size, size_t NumberOfMembers, void *pArgs) {
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

int main(int argc, char *argv[]) {
	if(argc < 2){
		fprintf(stderr, "사용법: %s <도시명>\n", argv[0]);
		fprintf(stderr, "Examples:\n");
		fprintf(stderr, "\tWeatherFetch.exe \"서울\"\n\tWeatherFetch.exe \"인천\"\n\tWeatherFetch.exe \"수원\"\n");
		return -1;
	}

	double Latitude,
		   Longitude;
	char *CityCode = NULL;
	if(!FindCity(argv[1], &CityCode)){
		return -1;
	}
	GetForecastZone(CityCode, &Latitude, &Longitude);

	int MyPlaceX,
		MyPlaceY;
	LatLonToGrid(Latitude, Longitude, &MyPlaceX, &MyPlaceY);

	char Location[0x100];
	memset(Location, 0, sizeof(Location));
	GetLocationName(Latitude, Longitude, Location);

	CURL *Curl = curl_easy_init();
	if(!Curl){
		fprintf(stderr, "Curl Init Failed: %s\n", curl_easy_strerror(CURLE_FAILED_INIT));
		return -1;
	}

	struct xmlMemory Mem;
	Mem.Text = (char*)malloc(1);
	Mem.Size = 0;

	char URL[0x400];
	char *Date = GetDate();
	snprintf(URL, sizeof(URL), KMA_URL_WEATHER, KMA_API_KEY_WEATHER, NUMOFROWS1, PAGENO1, DATATYPE1, Date, BASETIME_TABLE[Aprx(CurrentTime())], MyPlaceX, MyPlaceY);
	free(Date);

	URL[strlen(URL)] = 0;
	curl_easy_setopt(Curl, CURLOPT_URL, URL);
	curl_easy_setopt(Curl, CURLOPT_WRITEFUNCTION, KMAWriteCallback);
	curl_easy_setopt(Curl, CURLOPT_WRITEDATA, &Mem);
	// curl_easy_setopt(Curl, CURLOPT_SSL_VERIFYPEER, 1);

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
							const xmlChar* Unit = NULL;
							const xmlChar* BaseDate = NULL;
							const xmlChar* BaseTime = NULL;
							const xmlChar* CategoryValue = NULL;
							const xmlChar* FcstDate = NULL;
							const xmlChar* FcstTime = NULL;
							const xmlChar* FcstValue = NULL;

							xmlNode *Data = Item->children;
							while(Data){
								const xmlChar* Name = (const xmlChar*)Data->name;

								if(Data->type == XML_ELEMENT_NODE){
									if(xmlStrcmp(Name, (const xmlChar*)"baseDate") == 0){
										BaseDate = xmlNodeGetContent(Data);
										goto Next;
									}
									if(xmlStrcmp(Name, (const xmlChar*)"baseTime") == 0){
										BaseTime = xmlNodeGetContent(Data);
										goto Next;
									}
									if(xmlStrcmp(Name, (const xmlChar*)"category") == 0){
										for(int i=0; i<sizeof(CategoryTable) / sizeof(CategoryTable[0]); i++){
											if(xmlStrcmp(xmlNodeGetContent(Data), (const xmlChar*)CategoryTable[i].Category) == 0){
												CategoryValue = (const xmlChar*)CategoryTable[i].Mean;
												Unit = (const xmlChar*)CategoryTable[i].Unit;
												break;
											}
										}
										goto Next;
									}
									if(xmlStrcmp(Name, (const xmlChar*)"fcstDate") == 0){
										FcstDate = xmlNodeGetContent(Data);
										goto Next;
									}
									if(xmlStrcmp(Name, (const xmlChar*)"fcstTime") == 0){
										FcstTime = xmlNodeGetContent(Data);
										goto Next;
									}
									if(xmlStrcmp(Name, (const xmlChar*)"fcstValue") == 0){
										FcstValue = xmlNodeGetContent(Data);
										goto Next;
									}
								}

								Next:
									Data = Data->next;
							}

							printf("----------------------\n");
							printf("BaseDate: %s\n", BaseDate);
							printf("BaseTime: %s\n", BaseTime);
							printf("Category: %s\n", CategoryValue);
							printf("FcstDate: %s\n", FcstDate);
							printf("FcstTime: %s\n", FcstTime);
							printf("FcstValue: %s %s\n", FcstValue, Unit);
							printf("Location Name: %s\n", Location);
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

