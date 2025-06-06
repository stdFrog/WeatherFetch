이 프로젝트는 기상청(KMA)에서 공개하는 OpenAPI를 활용해 날씨 정보를 가져오는 싱글 스레드 콘솔 프로그램입니다.
curl 라이브러리를 이용해 예보구역정보와 단기예보 정보를 요청하고, 받아온 정보를 libxml2와 cJSON 라이브러리로 파싱 및 포맷팅하여 날씨 정보를 출력합니다.

메인 함수 선두를 보면 위경도 값을 가져오기 위해 CityCode를 필요로 합니다.
이 CityCode는 예보구역정보 문서에 공개되어 있으니 참고하시기 바랍니다.

CityCode를 이용해 위경도 값을 조회하고 다시 이 값을 기상청에서 사용하는 격자값(X,Y)으로 변환하며, URL을 조립한 후 단기예보 정보를 받아옵니다.

위경도 값과 기상청에서 사용하는 격자값은 서로간 변환할 수 있으며, 기상청에서 유틸리티 함수를 제공합니다(단기예보 문서 참고).

받아온 xml 문서를 탐색하며 구문 분석 후 데이터를 조립하여 일정한 포맷으로 출력하는데 단기예보 정보를 요청하기 전에 GetLocationName 함수를 호출합니다.

이 함수는 Google Maps에서 제공하는 GEOCODING API를 사용하며 대략적인 예보 구역 이름으로부터 구체적인 주소 정보를 가져오기 위해 필요합니다.

프로그램의 실행 결과는 다음과 같습니다.

----------------------
BaseDate: 20250607
BaseTime: 0200
Category: Hourly Temperature
FcstDate: 20250607
FcstTime: 0300
FcstValue: 20 (Celcius)
Location Name: 54-3 Taepyeongno 1(il)-ga, Jung District, Seoul, South Korea
----------------------

기준 시각과 날짜, 조사 시각과 날짜는 자동 선택할 수 있도록 코드를 작성해뒀습니다.
함수는 대체로 어렵지 않아 따로 분석하지는 않겠습니다.

프로젝트에서 사용된 OpenAPI 관련 링크를 남기고 이만 줄이겠습니다.
- 단기예보: https://www.data.go.kr/data/15084084/openapi.do
- 예보구역정보: https://www.data.go.kr/data/15057111/openapi.do
- GoogleMaps Geocoding API: https://developers.google.com/maps/get-started?hl=ko

참고로 기상청과 GoogleMaps 모두 회원가입을 해야하며 API를 사용하기 위해 신뢰할 수 있는 인증 정보(사용자 정보)가 필요합니다.
GoogleMaps는 로그인을 하지 않으면 API 라이브러리 검색도 되지 않아 시작 페이지의 링크를 첨부해뒀습니다.
위 링크에서 "프로젝트 선택기 페이지로 이동"을 클릭하여 프로젝트를 만들고 Geocoding API를 검색하여 "사용" 설정 후 API 키를 복사하여 사용하면 됩니다.

예시 헤더 샘플을 올려둘 예정이니 참고하시기 바랍니다.
