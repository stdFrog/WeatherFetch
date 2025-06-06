#ifndef __RESOURCE_H_
#define __RESOURCE_H_

#define KMA_API_KEY_WEATHER			"YOUR_KMA_API_KEY1"
#define KMA_API_KEY_FORECAST_ZONE	"YOUR_KMA_API_KEY2"
#define GEOCODING_API_KEY			"YOUR_GEOCODING_API_KEY"


#define KMA_URL_WEATHER				"https://apis.data.go.kr/1360000/VilageFcstInfoService_2.0/getVilageFcst?serviceKey=%s&numOfRows=%d&pageNo=%d&dataType=%s&base_date=%s&base_time=%02d00&nx=%d&ny=%d"
#define KMA_URL_FORECAST_ZONE		"http://apis.data.go.kr/1360000/FcstZoneInfoService/getFcstZoneCd?serviceKey=%s&numOfRows=%d&pageNo=%d&dataType=%s&regId=%s"
#define GEOCODING_URL				"https://maps.googleapis.com/maps/api/geocode/json?latlng=%f,%f&key=%s"

#define NUMOFROWS1	50
#define NUMOFROWS2	10
#define PAGENO1		1
#define PAGENO2		2
#define DATATYPE1	"XML"
#define DATATYPE2	"JSON"

const int BASETIME_TABLE[] = { 2, 5, 8, 11, 14, 17, 20, 23 };

struct CityCode{
	const char Name[0x100];
	const char Code[0x100];
};

struct CityCode CityList[] = {
	{ "서울", "11B10101" },
	...
};
#endif
