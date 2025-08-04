#include <3ds.h>
#include <citro2d.h>
#include <jansson.h>
#include "scene_mainmenu.h"
#include "scene_manager.h"
#include "main.h"
#include "cwav_shit.h"
#include "sprites.h"
#include "request.h"
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

float bounceScale = 1.0f;

extern CWAVInfo cwavList[8];
CWAV* title;
C2D_TextBuf MainMenu;
C2D_TextBuf Other;
C2D_Text menu_Text[50];
C2D_Text other_Text[50];
bool parsing = false;
SpriteAnimState test_state = {0, 0, 0, 5, false, 0, 0, 0, false};
char *gagatek = NULL;
ResponseBuffer top30_vtubers = {NULL, 0, false};
ResponseBuffer daily_plvtdle = {NULL, 0, false};
ResponseBuffer all_vtubers = {NULL, 0, false};
ResponseBuffer pfp_image = {NULL, 0, false};
bool endless;
bool git;
const char *gagatek_race = NULL;
const char *gagatek_eyes = NULL;
const char *gagatek_hair = NULL;
const char *gagatek_gender = NULL;
int gagatek_year;
int gagatek_height;
bool raceMatch = false;
bool eyesMatch = false;
bool hairMatch = false;
bool genderMatch = false;
bool yearMatch = false;
bool heightMatch = false;
float outHeight;
bool pressed_Once;
float names_Y;
float target_names_Y = 120.0f;
int choose = 0;
bool parsed_once;
bool parsed_daily;
u32 totalSamples;
typedef struct {
    const char **names;
    size_t count;
	bool parsed;
	bool *checked; 
	const char **gender;	
	const char **race;	
	const char **hair;	
	const char **eyes;	
	int *height;
	int *year;
	bool *image_loaded;
	const char **pfp_url;
	const char **vtuber_id;
} NameList;
CWAV* menumusic;
NameList lista;
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240
#define NUM_POINTS 30
#define BASE_HEIGHT (SCREEN_HEIGHT / 2)
#define WAVE_AMPLITUDE 10.0f
#define WAVE_FREQUENCY 0.025f
#define EASE_DURATION 0.5f 
#define deltaTime (1.0f / 60.0f)
static float gitTimer = 0.0f;
#define STAR_COUNT 15
#define SCREEN_TOP_Y 0
#define SCREEN_BOTTOM_Y 240
#define STAR_LAYERS 3


typedef struct {
    float x, y;
    float speedX, speedY;
    float scale;     
	float angleTimer;
} Star;

Star stars[STAR_COUNT];


void resetStar(Star* star, float scale) {
    star->scale = scale;
	star->angleTimer = 0;
    star->x = (rand() % SCREEN_WIDTH) + 30;
    star->y = -16;

    star->speedX = (-1.0f - (rand() % 2)) * scale;
    star->speedY = (1.0f + (rand() % 2)) * scale;
}

void initStars() {
    for (int i = 0; i < STAR_COUNT; i++) {
        
        float scale;
        int layer = i % STAR_LAYERS;
        switch(layer) {
            case 0: scale = 0.25f; break;
            case 1: scale = 0.3f; break;
            case 2: scale = 0.4f; break;  
        }
        resetStar(&stars[i], scale);
    }
}

void updateStars() {
    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].x += stars[i].speedX;
        stars[i].y += stars[i].speedY;

        if (stars[i].y > SCREEN_HEIGHT + 45 || stars[i].x < -16) {
            resetStar(&stars[i], stars[i].scale);
        }
    }
}
// Somewhere global or static
float angleTimer = 0.0f;
void drawStars(C2D_Image* image) {
    float halfW = image->subtex->width / 2.0f;
    float halfH = image->subtex->height / 2.0f;


    C2D_ImageTint starTint;
    C2D_AlphaImageTint(&starTint, 0.5f);


    C2D_ImageTint shadowTint;
    C2D_PlainImageTint(&shadowTint, C2D_Color32(0, 0, 0, 190), 1.0f);

    for (int i = 0; i < STAR_COUNT; i++) {
        float sx = stars[i].x - halfW * stars[i].scale;
        float sy = stars[i].y - halfH * stars[i].scale;


        stars[i].angleTimer += 0.05f;


        float rawSine = sinf(stars[i].angleTimer);
        float eased = rawSine * rawSine * (rawSine < 0 ? -1.0f : 1.0f);
        float angle = eased * 0.4f;


        float shadowOffset = 3.0f; 
        float shadowScale = stars[i].scale * 0.95f; 
        C2D_DrawImageAtRotated(
            *image,
            sx + shadowOffset, sy + shadowOffset,
            0.05f,                
            angle,
            &shadowTint,
            shadowScale,
            shadowScale
        );


        C2D_DrawImageAtRotated(
            *image,
            sx, sy,
            0.1f,
            angle,
            &starTint,
            stars[i].scale,
            stars[i].scale
        );
    }
}

typedef struct {
    float x, y;
} WavePoint;
#define ITEMS_PER_PAGE 30
#define LINE_HEIGHT 20.0f  // Same as your 20.0f multiplier
WavePoint wave[NUM_POINTS];
float phaseOffsets[NUM_POINTS];

void initWaveOffsets() {
    srand(time(NULL));
    for (int i = 0; i < NUM_POINTS; ++i) {
        phaseOffsets[i] = ((rand() % 1000) / 1000.0f) * 2.0f * M_PI;
    }
}
extern bool czasfuckup;
void updateWave(float time) {
    const float t1 = time * 20.0f;
    const float t2 = time * 12.0f;
    const float t3 = time * 7.0f;
    const float waveFreq2 = WAVE_FREQUENCY * 2.0f;
    const float waveFreq05 = WAVE_FREQUENCY * 0.5f;

    for (int i = 0; i < NUM_POINTS; ++i) {
        float norm = (float)i / (NUM_POINTS - 1);
        float x = norm * SCREEN_WIDTH;
        float phase = x + phaseOffsets[i];

        float y = BASE_HEIGHT
                + sinf(WAVE_FREQUENCY * (phase + t1)) * WAVE_AMPLITUDE
                + sinf(waveFreq2 * (x + t2)) * (WAVE_AMPLITUDE * 0.5f)
                + sinf(waveFreq05 * (x + t3)) * (WAVE_AMPLITUDE * 0.3f);

        y += (((float)(rand() % 100) / 100.0f) - 0.5f) * 0.9f;

        wave[i].x = x;
        wave[i].y = y;
    }
}

void drawWaveFill() {
    u32 fillColor = C2D_Color32(210, 210, 210, 40);

    float mid = NUM_POINTS / 2.0f;

    for (int i = 1; i < NUM_POINTS; ++i) {
        float x0 = wave[i - 1].x;
        float y0 = wave[i - 1].y;
        float x1 = wave[i].x;
        float y1 = wave[i].y;

        float curve0 = 1.0f - fabsf((i - 1) - mid) / mid;
        float curve1 = 1.0f - fabsf(i - mid) / mid;


        y0 += 30.0f * (1.0f - bounceScale) * curve0;
        y1 += 30.0f * (1.0f - bounceScale) * curve1;

        C2D_DrawTriangle(x0, y0, fillColor,
                         x1, y1, fillColor,
                         x0, SCREEN_HEIGHT, fillColor, 0.5f);

        C2D_DrawTriangle(x1, y1, fillColor,
                         x1, SCREEN_HEIGHT, fillColor,
                         x0, SCREEN_HEIGHT, fillColor, 0.5f);
    }
}


void fix_image_size(const char *input_url, char *output_url, size_t output_size) {
    const char *size_marker = "-profile_image-";
    const char *pos = strstr(input_url, size_marker);

    if (pos) {

        const char *ext = strrchr(input_url, '.');
        if (!ext || strchr(ext, '/') != NULL) {
            ext = ".png"; 
        }


        size_t prefix_len = pos - input_url + strlen(size_marker);


        size_t required_len = prefix_len + strlen("150x150") + strlen(ext) + 1;
        if (required_len >= output_size) {
            fprintf(stderr, "Output buffer too small.\n");
            return;
        }


        strncpy(output_url, input_url, prefix_len);
        output_url[prefix_len] = '\0';


        strcat(output_url, "150x150");
        strcat(output_url, ext);
    } else {

        strncpy(output_url, input_url, output_size - 1);
        output_url[output_size - 1] = '\0';
    }
}
size_t strona;
NameList parse_display_names_copy(const char *json_str) {
    NameList result = {NULL, 0, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL};

    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);
    if (!root) {
        log_to_file("Error parsing JSON: %s\n", error.text);
        return result;
    }

    json_t *data_array = json_object_get(root, "data");
    if (!json_is_array(data_array)) {
        log_to_file("Error: 'data' is not an array\n");
        json_decref(root);
        return result;
    }
    size_t array_size = json_array_size(data_array);

    const char **names = malloc(sizeof(char *) * array_size);
    const char **gender = malloc(sizeof(char *) * array_size);
    const char **race = malloc(sizeof(char *) * array_size);
    const char **hair = malloc(sizeof(char *) * array_size);
    const char **eyes = malloc(sizeof(char *) * array_size);
    int *height = malloc(sizeof(int) * array_size);
    int *year = malloc(sizeof(int) * array_size);
    bool *checked = calloc(array_size, sizeof(bool)); 
    bool *image_loaded = calloc(array_size, sizeof(bool)); 
    const char **pfp_url = malloc(sizeof(char *) * array_size);
    const char **vtuber_id = malloc(sizeof(char *) * array_size);

    if (!names || !gender || !race || !hair || !eyes || !height || !year || !checked || !image_loaded || !pfp_url || !vtuber_id) {
        free(names); free(gender); free(race); free(hair); free(eyes);
        free(height); free(year); free(checked); free(image_loaded); free(pfp_url); free(vtuber_id);
        json_decref(root);
        return result;
    }

    const char *fallback_image_url = "https://barachlo.szprink.xyz/fiIAlnosGV5w.png";

    size_t count = 0;
    for (size_t i = 0; i < array_size; i++) {
        json_t *entry = json_array_get(data_array, i);
        json_t *name_json = json_object_get(entry, "displayName");
        json_t *gender_json = json_object_get(entry, "gender");
        json_t *race_arr = json_object_get(entry, "race");
        json_t *hair_arr = json_object_get(entry, "hair");
        json_t *eyes_arr = json_object_get(entry, "eyes");
        json_t *height_json = json_object_get(entry, "height");
        json_t *year_json = json_object_get(entry, "year");
        json_t *image_json = json_object_get(entry, "image");
        json_t *vtube_id_json = json_object_get(entry, "_id");

        if (json_is_string(name_json)) {
            const char *name_str = json_string_value(name_json);
            const char *id_str = json_string_value(vtube_id_json);

            const char *image_str = NULL;
            if (json_is_string(image_json)) {
                image_str = json_string_value(image_json);
            }

            char fixed_image_url[512];
            if (image_str && strlen(image_str) > 0) {
                fix_image_size(image_str, fixed_image_url, sizeof(fixed_image_url));
            } else {
                log_to_file("Warning: entry %zu missing or invalid 'image', using fallback URL\n", i);
                strncpy(fixed_image_url, fallback_image_url, sizeof(fixed_image_url) - 1);
                fixed_image_url[sizeof(fixed_image_url) - 1] = '\0';
            }

            const char *gender_str = json_is_string(gender_json) ? json_string_value(gender_json) : "unknown";

            // Race concat with ignoring unknown logging
            char race_concat[256] = {0};
            if (json_is_array(race_arr) && json_array_size(race_arr) > 0) {
                size_t race_count = json_array_size(race_arr);
                for (size_t r = 0; r < race_count; r++) {
                    json_t *race_item = json_array_get(race_arr, r);
                    if (json_is_string(race_item)) {
                        const char *race_str_tmp = json_string_value(race_item);
                        if (strlen(race_concat) > 0) {
                            strncat(race_concat, ", ", sizeof(race_concat) - strlen(race_concat) - 1);
                        }
                        strncat(race_concat, race_str_tmp, sizeof(race_concat) - strlen(race_concat) - 1);
                    }
                }
                if (strlen(race_concat) == 0) {
                    strncpy(race_concat, "unknown", sizeof(race_concat) - 1);
                }
            } else {
                strncpy(race_concat, "unknown", sizeof(race_concat) - 1);
            }

            // Hair concat with ignoring unknown logging
            char hair_concat[256] = {0};
            if (json_is_array(hair_arr) && json_array_size(hair_arr) > 0) {
                size_t hair_count = json_array_size(hair_arr);
                for (size_t h = 0; h < hair_count; h++) {
                    json_t *hair_item = json_array_get(hair_arr, h);
                    if (json_is_string(hair_item)) {
                        const char *hair_str_tmp = json_string_value(hair_item);
                        if (strlen(hair_concat) > 0) {
                            strncat(hair_concat, ", ", sizeof(hair_concat) - strlen(hair_concat) - 1);
                        }
                        strncat(hair_concat, hair_str_tmp, sizeof(hair_concat) - strlen(hair_concat) - 1);
                    }
                }
                if (strlen(hair_concat) == 0) {
                    strncpy(hair_concat, "unknown", sizeof(hair_concat) - 1);
                }
            } else {
                strncpy(hair_concat, "unknown", sizeof(hair_concat) - 1);
            }

            // Eyes concat with ignoring unknown logging
            char eyes_concat[512] = {0};
            if (json_is_array(eyes_arr) && json_array_size(eyes_arr) > 0) {
                size_t eyes_count = json_array_size(eyes_arr);
                for (size_t j = 0; j < eyes_count; j++) {
                    json_t *eye_type = json_array_get(eyes_arr, j);
                    if (json_is_string(eye_type)) {
                        const char *eye_str = json_string_value(eye_type);
                        if (strlen(eyes_concat) > 0) {
                            strncat(eyes_concat, ", ", sizeof(eyes_concat) - strlen(eyes_concat) - 1);
                        }
                        strncat(eyes_concat, eye_str, sizeof(eyes_concat) - strlen(eyes_concat) - 1);
                    }
                }
                if (strlen(eyes_concat) == 0) {
                    strncpy(eyes_concat, "unknown", sizeof(eyes_concat) - 1);
                }
            } else {
                strncpy(eyes_concat, "unknown", sizeof(eyes_concat) - 1);
            }

            int height_int = 0;
            if (json_is_integer(height_json)) {
                height_int = json_integer_value(height_json);
            } else {
                log_to_file("Notice: entry %zu missing or invalid 'height', using 0\n", i);
                height_int = 0;
            }

            int year_int = 0;
            if (json_is_integer(year_json)) {
                year_int = json_integer_value(year_json);
            } else {
                // You said just put 0 for missing, no need to log
                year_int = 0;
            }

            names[count] = strdup(name_str);
            vtuber_id[count] = strdup(id_str);
            pfp_url[count] = strdup(fixed_image_url);
            gender[count] = strdup(gender_str);
            race[count] = strdup(race_concat);
            hair[count] = strdup(hair_concat);
            eyes[count] = strdup(eyes_concat);
            height[count] = height_int;
            year[count] = year_int;
            checked[count] = false;
            image_loaded[count] = false;
            count++;
        }
    }

    json_decref(root);

    result.names = names;
    result.vtuber_id = vtuber_id;
    result.pfp_url = pfp_url;
    result.checked = checked;
    result.image_loaded = image_loaded;
    result.count = count;
    result.gender = gender;
    result.race = race;
    result.hair = hair;
    result.eyes = eyes;
    result.height = height;
    result.year = year;
    result.parsed = true;
    return result;
}

void freeNameList(NameList *list) {
    if (!list || !list->parsed) return;

    free(list->names);
    free(list->checked);
    free(list->gender);
    free(list->race);
    free(list->hair);
    free(list->eyes);
    free(list->height);
    free(list->year);
    free(list->image_loaded);
    free(list->pfp_url);
    free(list->vtuber_id);

    // Optional: reset fields
    list->names = NULL;
    list->count = 0;
    list->parsed = false;
}
bool parse_daily_vtuber(const char *json_str) {
    if (!gagatek) {
		log_to_file("JSON: %s\n", json_str);
        json_error_t error;
        json_t *root = json_loads(json_str, 0, &error);
        if (!root) {

            return false; 
        }
		
		json_t *data = json_object_get(root, "data");
        json_t *vtuberId = json_object_get(data, "vtuberId");
        const char *loginStr = json_string_value(vtuberId);

        if (loginStr) {
            gagatek = strdup(loginStr);
        }

        json_decref(root);
		parsed_daily = true;
    }

    return gagatek != NULL; 
}
bool get_random_vtuber_id(const char *json_text) {
    if (gagatek) return true;  // Already set

    json_error_t error;
    json_t *root = json_loads(json_text, 0, &error);
    if (!root) return false;

    json_t *data_array = json_object_get(root, "data");
    if (!json_is_array(data_array)) {
        json_decref(root);
        return false;
    }

    size_t array_size = json_array_size(data_array);
    if (array_size == 0) {
        json_decref(root);
        return false;
    }

    srand(time(NULL));
    size_t index = rand() % array_size;

    json_t *vtuber = json_array_get(data_array, index);
    json_t *id = json_object_get(vtuber, "id");

    if (json_is_string(id)) {
        gagatek = strdup(json_string_value(id));
    }
	parsed_daily = true;
    json_decref(root);
    return gagatek != NULL;
}

void draw_names(const NameList *list, C2D_TextBuf textBuf, float start_x, float start_y, float line_height, int strona, int choose) {
    C2D_Text text;
    float y = start_y;

    size_t start = strona * ITEMS_PER_PAGE;
    size_t end = start + ITEMS_PER_PAGE;
    if (end > list->count) end = list->count;

    for (size_t i = start; i < end; i++) {
        C2D_TextParse(&text, textBuf, list->names[i]);
        C2D_TextOptimize(&text);

        uint32_t color = (list->checked[i])
            ? C2D_Color32(255, 255, 255, 120)
            : C2D_Color32(255, 255, 255, 255);

        if ((int)i == choose) {
            // Add a subtle background or highlight if it's selected (optional)
            C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, start_x - 3, y + 3, 1.0f, 0.9f, 0.9f, C2D_Color32(0, 0, 0, 160));
            C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, start_x, y, 1.0f, 0.9f, 0.9f, color);
        } else {
            C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, start_x - 3, y + 3, 1.0f, 0.9f, 0.9f, C2D_Color32(0, 0, 0, 140));
            C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, start_x, y, 1.0f, 0.9f, 0.9f, color);
        }

        y += line_height;
    }
}
void getVTuberDaily(void){
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    int intyear = tm_info->tm_year + 1900;  
    int intmonth = tm_info->tm_mon + 1;   
    char year[20];

    sprintf(year, "%d", intyear);	
    char month[20];

    sprintf(month, "%02d", intmonth - 1);	
	char timeurl[512];
	snprintf(timeurl, sizeof(timeurl), "https://plvtuber.pl/api/vtuber/top?metric=avgViewers&limit=30&month=%s-%s", year, month);
	queue_request("https://plvtuber.pl/api/plvtdle/daily/match", "", NULL, &daily_plvtdle, false);
    queue_request(timeurl, "", NULL, &top30_vtubers, false);
	
}
u64 startTime;
float sampleRate;
static float fadeTimer = 0.0f;
#define FADE_DURATION 1.0f  
static bool fadeStarted = false;
void sceneMainMenuInit(void) {
	start_request_thread();
	MainMenu = C2D_TextBufNew(2048);
	Other = C2D_TextBufNew(2048);
    C2D_TextBufClear(MainMenu);  
	C2D_TextBufClear(Other); 
	C2D_TextParse(&other_Text[0], Other, "PLVTdle Demo");
	C2D_TextParse(&other_Text[1], Other, "<");
	C2D_TextParse(&other_Text[2], Other, "");
	C2D_TextParse(&other_Text[3], Other, "Ładowanie...");
	C2D_TextParse(&other_Text[4], Other, "Zgaduj!");
	C2D_TextParse(&other_Text[5], Other, "(SELECT) - Endless Mode");
	names_Y = 120.0f;
	pressed_Once = false;
	menumusic = cwavList[1].cwav;
	initWaveOffsets();
	initStars();
	endless = false;
	getVTuberDaily();
	fadeStarted = true;
    cwavPlay(menumusic, 0, 1); 
	//log_to_file("CWAV samples: %u, sampleRate: %u\n", menumusic->numSamples, menumusic->sampleRate);

	C2D_TextGetDimensions(&other_Text[3], 0.9f, 0.9f, NULL, &outHeight);
	
}
bool str_contains_case_insensitive(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;
    size_t needle_len = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, needle_len) == 0)
            return true;
    }
    return false;
}

int my_strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = tolower((unsigned char)*s1);
        char c2 = tolower((unsigned char)*s2);
        if (c1 != c2) {
            return (unsigned char)c1 - (unsigned char)c2;
        }
        s1++;
        s2++;
    }
    return (unsigned char)tolower((unsigned char)*s1) - (unsigned char)tolower((unsigned char)*s2);
}
void storeGagatekAttributes() {
    if (!gagatek || !lista.parsed) return;
    for (size_t i = 0; i < lista.count; i++) {
        log_to_file("Checking: %s vs %s\n", lista.vtuber_id[i], gagatek);
        if (my_strcasecmp(lista.vtuber_id[i], gagatek) == 0) {
            log_to_file("Match found for %s\n", gagatek);
			gagatek_gender = lista.gender[i];
            gagatek_race = lista.race[i];
            gagatek_eyes = lista.eyes[i];
            gagatek_hair = lista.hair[i];
			gagatek_height = lista.height[i];
			gagatek_year = lista.year[i];
            break;
        }
    }
}


void checkVtubeDaily(const char *name){
	C2D_TextBufClear(Other);
	C2D_TextParse(&other_Text[1], Other, "<");
	C2D_TextParse(&other_Text[2], Other, "");
	C2D_TextParse(&other_Text[3], Other, "Ładowanie...");
	pfp_image.done = false;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: okhttp/4.12.0");
    headers = curl_slist_append(headers, "Accept: application/json");
	queue_request(lista.pfp_url[choose], "", headers, &pfp_image, true);
    git = (my_strcasecmp(gagatek, name) == 0);
	if (!git) {
		C2D_TextParse(&other_Text[2], Other, git ? "DOBRZE" : "ŹLE");

		C2D_TextParse(&other_Text[5], Other, lista.names[choose]);

		if (my_strcasecmp(lista.gender[choose], "female") == 0){
			C2D_TextParse(&other_Text[6], Other, "Kobieta");	
		} else {
			C2D_TextParse(&other_Text[6], Other, "Mężczyzna");	
		}
		genderMatch = (gagatek_gender && lista.gender[choose] && my_strcasecmp(lista.gender[choose], gagatek_gender) == 0);
		raceMatch = (gagatek_race && lista.race[choose] && my_strcasecmp(lista.race[choose], gagatek_race) == 0);
		eyesMatch = (gagatek_eyes && lista.eyes[choose] && my_strcasecmp(lista.eyes[choose], gagatek_eyes) == 0);
		hairMatch = (gagatek_hair && lista.hair[choose] && my_strcasecmp(lista.hair[choose], gagatek_hair) == 0);
		yearMatch = (lista.year[choose] == gagatek_year);
		heightMatch = (lista.height[choose] == gagatek_height);

		// log_to_file("Checking %s vs gagatek %s\n", name, gagatek);
		// log_to_file("Gender: %s vs %s, match=%d\n", lista.gender[choose], gagatek_gender, genderMatch);
		// log_to_file("Race: %s vs %s, match=%d\n", lista.race[choose], gagatek_race, raceMatch);
		// log_to_file("Eyes: %s vs %s, match=%d\n", lista.eyes[choose], gagatek_eyes, eyesMatch);
		// log_to_file("Hair: %s vs %s, match=%d\n", lista.hair[choose], gagatek_hair, hairMatch);


		char buffer[512];
		snprintf(buffer, sizeof(buffer), "Rasa: %s", lista.race[choose]);
		C2D_TextParse(&other_Text[7], Other, buffer);

		snprintf(buffer, sizeof(buffer), "Oczy: %s", lista.eyes[choose]);
		C2D_TextParse(&other_Text[8], Other, buffer);

		snprintf(buffer, sizeof(buffer), "Włosy: %s", lista.hair[choose]);
		C2D_TextParse(&other_Text[9], Other, buffer);
		
		snprintf(buffer, sizeof(buffer), "Wzrost: %dcm", lista.height[choose]);
		C2D_TextParse(&other_Text[10], Other, buffer);
		
		snprintf(buffer, sizeof(buffer), "Rok: %d", lista.year[choose]);
		C2D_TextParse(&other_Text[11], Other, buffer);
	} else {
		C2D_TextParse(&other_Text[2], Other, "Tak! To");
		C2D_TextParse(&other_Text[5], Other, lista.names[choose]);
	}
}
float timew;
#define ITEMS_PER_PAGE 30
#define LINE_HEIGHT 20.0f  // Same as your 20.0f multiplier
void sceneMainMenuUpdate(uint32_t kDown, uint32_t kHeld) {
    timew += 0.1f;
    updateWave(timew);
    updateStars();

    if (kDown & KEY_A && lista.parsed && !git) {
		CWAV* sfxc = cwavList[4].cwav;
		cwavPlay(sfxc, 0, 1);
        pressed_Once = true;
        lista.checked[choose] = true;
        checkVtubeDaily(lista.vtuber_id[choose]);
        bounceScale = 1.2f;  
		if (git){
			if (!endless) {
				C2D_TextParse(&other_Text[6], Other, "(SELECT) - Endless Mode");
			} else {
				C2D_TextParse(&other_Text[6], Other, "(SELECT) - Spróbuj Ponownie");
			}
		}
    }

    if (git) {
        gitTimer += deltaTime;
        if (gitTimer > EASE_DURATION) gitTimer = EASE_DURATION;
    } else {
        gitTimer -= deltaTime;
        if (gitTimer < 0.0f) gitTimer = 0.0f;
    }

    bounceScale += (1.0f - bounceScale) * 0.2f;
	// PAGE RIGHT (go 1 page forward)
	if (kDown & KEY_R && !git && endless) {
		int nextPageStart = (strona + 1) * ITEMS_PER_PAGE;

		if (nextPageStart < (int)lista.count) {
			choose = nextPageStart;

			while (choose < (int)lista.count && lista.checked[choose]) {
				choose++;
			}

			int pageEnd = nextPageStart + ITEMS_PER_PAGE;
			if (pageEnd > (int)lista.count) pageEnd = (int)lista.count;

			if (choose >= pageEnd) {
				choose = pageEnd - 1;
				while (choose >= nextPageStart && lista.checked[choose]) {
					choose--;
				}
				if (choose < nextPageStart) {
					choose = 0;
					while (choose < (int)lista.count && lista.checked[choose]) {
						choose++;
					}
					if (choose >= (int)lista.count) choose = lista.count - 1;
					strona = choose / ITEMS_PER_PAGE;
					if (pressed_Once) {
						target_names_Y = 120.0f - LINE_HEIGHT * (choose - strona * ITEMS_PER_PAGE);
					} else {
						target_names_Y = 120.0f;
					}
					return;
				}
			}
			strona++;  // Move to next page normally

			if (pressed_Once) {
				target_names_Y = 120.0f - LINE_HEIGHT * (choose - strona * ITEMS_PER_PAGE);
			} else {
				target_names_Y = 120.0f;
			}
			CWAV* sfxc = cwavList[3].cwav;
			cwavPlay(sfxc, 0, 1);

		} else {
			int lastPageStart = ((lista.count - 1) / ITEMS_PER_PAGE) * ITEMS_PER_PAGE;
			choose = lastPageStart;

			while (choose < (int)lista.count && lista.checked[choose]) {
				choose++;
			}
			if (choose >= (int)lista.count) {
				choose = lista.count - 1;
			}
			strona = lastPageStart / ITEMS_PER_PAGE;

			if (pressed_Once) {
				target_names_Y = 120.0f - LINE_HEIGHT * (choose - strona * ITEMS_PER_PAGE);
			} else {
				target_names_Y = 120.0f;
			}
		}
	}

	// PAGE LEFT (go 1 page backward)
	if (kDown & KEY_L && !git && endless) {
		int prevPageStart = (strona - 1) * ITEMS_PER_PAGE;

		if (prevPageStart >= 0) {
			choose = prevPageStart;

			while (choose < (int)lista.count && lista.checked[choose]) {
				choose++;
			}

			int pageEnd = prevPageStart + ITEMS_PER_PAGE;
			if (pageEnd > (int)lista.count) pageEnd = (int)lista.count;

			if (choose >= pageEnd) {
				choose = pageEnd - 1;
				while (choose >= prevPageStart && lista.checked[choose]) {
					choose--;
				}
				if (choose < prevPageStart) {
					choose = 0;
					while (choose < (int)lista.count && lista.checked[choose]) {
						choose++;
					}
					if (choose >= (int)lista.count) choose = lista.count - 1;
					strona = choose / ITEMS_PER_PAGE;
					if (pressed_Once) {
						target_names_Y = 120.0f - LINE_HEIGHT * (choose - strona * ITEMS_PER_PAGE);
					} else {
						target_names_Y = 120.0f;
					}
					return;
				}
			}
			strona--;

			if (pressed_Once) {
				target_names_Y = 120.0f - LINE_HEIGHT * (choose - strona * ITEMS_PER_PAGE);
			} else {
				target_names_Y = 120.0f;
			}
			CWAV* sfxc = cwavList[3].cwav;
			cwavPlay(sfxc, 0, 1);

		} else {
			choose = 0;
			while (choose < (int)lista.count && lista.checked[choose]) {
				choose++;
			}
			if (choose >= (int)lista.count) {
				choose = lista.count - 1;
			}
			strona = 0;

			if (pressed_Once) {
				target_names_Y = 120.0f - LINE_HEIGHT * (choose - strona * ITEMS_PER_PAGE);
			} else {
				target_names_Y = 120.0f;
			}
		}
	}
	if (kDown & KEY_DRIGHT && !git && endless) {
		int baseChoose = choose + 10;
		if (baseChoose >= (int)lista.count) baseChoose = lista.count - 1;

		int newChoose = baseChoose;

		// If baseChoose is checked, try moving forward to find unchecked
		if (lista.checked[newChoose]) {
			int temp = newChoose;
			while (temp < (int)lista.count && lista.checked[temp]) {
				temp++;
			}
			// Use found unchecked or fallback to baseChoose
			newChoose = (temp < (int)lista.count) ? temp : baseChoose;
		}

		choose = newChoose;
		strona = choose / ITEMS_PER_PAGE;


		target_names_Y = 120.0f - LINE_HEIGHT * (choose - strona * ITEMS_PER_PAGE);

		CWAV* sfxc = cwavList[3].cwav;
		cwavPlay(sfxc, 0, 1);
	}


	if (kDown & KEY_DLEFT && !git && endless) {
		int baseChoose = choose - 10;
		if (baseChoose < 0) baseChoose = 0;

		int newChoose = baseChoose;

		// If baseChoose is checked, try moving backward to find unchecked
		if (lista.checked[newChoose]) {
			int temp = newChoose;
			while (temp >= 0 && lista.checked[temp]) {
				temp--;
			}
			// Use found unchecked or fallback to baseChoose
			newChoose = (temp >= 0) ? temp : baseChoose;
		}

		choose = newChoose;
		strona = choose / ITEMS_PER_PAGE;


		target_names_Y = 120.0f - LINE_HEIGHT * (choose - strona * ITEMS_PER_PAGE);

		CWAV* sfxc = cwavList[3].cwav;
		cwavPlay(sfxc, 0, 1);
	}



	// MOVE UP within current page, skip checked
	if (kDown & KEY_DUP && !git) {
		int pageStart = strona * ITEMS_PER_PAGE;
		int oldChoose = choose;
		int newChoose = choose - 1;

		while (newChoose >= pageStart && lista.checked[newChoose]) {
			newChoose--;
		}

		if (newChoose >= pageStart) {
			int delta = oldChoose - newChoose;
			choose = newChoose;
			target_names_Y += LINE_HEIGHT * delta;  // Scroll down visually as you move up logically
			CWAV* sfxc = cwavList[3].cwav;
			cwavPlay(sfxc, 0, 1);
		}
	}

	// MOVE DOWN within current page, skip checked
	if (kDown & KEY_DDOWN && !git) {
		int pageEnd = (strona + 1) * ITEMS_PER_PAGE;
		if (pageEnd > (int)lista.count) pageEnd = (int)lista.count;

		int oldChoose = choose;
		int newChoose = choose + 1;

		while (newChoose < pageEnd && lista.checked[newChoose]) {
			newChoose++;
		}

		if (newChoose < pageEnd) {
			int delta = newChoose - oldChoose;
			choose = newChoose;
			target_names_Y -= LINE_HEIGHT * delta;  // Scroll up visually as you move down logically
			CWAV* sfxc = cwavList[3].cwav;
			cwavPlay(sfxc, 0, 1);
		}
	}


    if (kDown & KEY_SELECT) {
		git = false;
		gitTimer = 0.0f;
        freeNameList(&lista);
        pressed_Once = false;
        parsing = false;
        endless = true;
		C2D_TextBufClear(Other);  
		free(gagatek);
		gagatek = NULL;
		C2D_TextParse(&other_Text[0], Other, "PLVTdle Demo");
		C2D_TextParse(&other_Text[1], Other, "<");
		C2D_TextParse(&other_Text[2], Other, "");
		C2D_TextParse(&other_Text[3], Other, "Ładowanie...");
		C2D_TextParse(&other_Text[4], Other, "Zgaduj!");
		parsed_daily = false;
		parsed_once = false;
		if (!all_vtubers.done){
			queue_request("https://plvtuber.pl/api/vtuber", "", NULL, &all_vtubers, false);
		}
    }

    if (!endless) {
        if (top30_vtubers.done && !parsing) {
            parsing = true;
            lista = parse_display_names_copy(top30_vtubers.data);
        }
    } else {
        if (all_vtubers.done && !parsing) {
            parsing = true;
            lista = parse_display_names_copy(all_vtubers.data);
        }        
    }

    float speed = 5.0f;  
    names_Y += (target_names_Y - names_Y) * 0.2f;

    if (!parsed_once && parsed_daily) {
        parsed_once = true;
        storeGagatekAttributes();
    }

    if (fadeStarted) {
        fadeTimer += 1.0f / 60.0f;
        if (fadeTimer >= FADE_DURATION) {
            fadeTimer = FADE_DURATION;
        }
    }
}



void sceneMainMenuRender(void) {
    C2D_SceneBegin(top);
    C2D_TargetClear(top, C2D_Color32(18, 18, 18, 255));

    if (!pressed_Once) {
        if (!lista.parsed) {
            C2D_DrawImageAt(vtubelogo, 0.0f, 0.0f, 0.0f, NULL, 1.0f, 1.0f);
            C2D_DrawText(&other_Text[0], C2D_WithColor | C2D_AlignCenter, 200.0f, 160.0f, 1.0f, 1.0f, 1.0f, C2D_Color32(255,255,255,255));
        } else {	
			drawStars(&flyinglogo);
			drawWaveFill();

            C2D_DrawText(&other_Text[4], C2D_WithColor | C2D_AlignCenter, 200.0f, 120.0f - outHeight, 1.0f, 1.5f, 1.5f, C2D_Color32(255,255,255,255));
			if (!endless){
				C2D_DrawText(&other_Text[5], C2D_WithColor | C2D_AlignCenter, 200.0f, 180.0f - outHeight, 1.0f, 0.9f, 0.9f, C2D_Color32(255,255,255,255));
			}
        }
    } else {	
		drawStars(&flyinglogo);
		drawWaveFill();
        if (git) {
			// C2D_DrawText(&other_Text[2], C2D_WithColor | C2D_AlignRight, 393.9f, 6.0f, 1.0f, bounceScale * 1.5f, bounceScale * 1.5f, C2D_Color32(0,0,0,255));
            // C2D_DrawText(&other_Text[2], C2D_WithColor | C2D_AlignRight, 395.0f, 5.0f, 1.0f, bounceScale * 1.5f, bounceScale * 1.5f, C2D_Color32(0,178,20,255));
        } else {
			C2D_DrawText(&other_Text[2], C2D_WithColor | C2D_AlignRight, 393.9f, 6.0f, 1.0f, bounceScale * 1.5f, bounceScale * 1.5f, C2D_Color32(0,0,0,255));
            C2D_DrawText(&other_Text[2], C2D_WithColor | C2D_AlignRight, 395.0f, 5.0f, 1.0f, bounceScale * 1.5f, bounceScale * 1.5f, C2D_Color32(204,2,112,255));
        }
    }
	if (!git) {
		if (pressed_Once){
			C2D_DrawText(&other_Text[5], C2D_WithColor | C2D_AlignLeft, 3.9f, 6.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 git ? C2D_Color32(0,0,0,255) : C2D_Color32(0,0,0,255));
			C2D_DrawText(&other_Text[6], C2D_WithColor | C2D_AlignLeft, 3.9f, 26.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 genderMatch ? C2D_Color32(0,0,0,255) : C2D_Color32(0,0,0,255));
			C2D_DrawText(&other_Text[7], C2D_WithColor | C2D_AlignLeft, 3.9f, 46.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 raceMatch ? C2D_Color32(0,0,0,255) : C2D_Color32(0,0,0,255));

			C2D_DrawText(&other_Text[8], C2D_WithColor | C2D_AlignLeft, 3.9f, 66.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 eyesMatch ? C2D_Color32(0,0,0,255) : C2D_Color32(0,0,0,255));

			C2D_DrawText(&other_Text[9], C2D_WithColor | C2D_AlignLeft, 3.9f, 86.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 hairMatch ? C2D_Color32(0,0,0,255) : C2D_Color32(0,0,0,255));
						 
			C2D_DrawText(&other_Text[10], C2D_WithColor | C2D_AlignLeft, 3.9f, 106.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 heightMatch ? C2D_Color32(0,0,0,255) : C2D_Color32(0,0,0,255));

			C2D_DrawText(&other_Text[11], C2D_WithColor | C2D_AlignLeft, 3.9f, 126.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 yearMatch ? C2D_Color32(0,0,0,255) : C2D_Color32(0,0,0,255));
						 
			C2D_DrawText(&other_Text[5], C2D_WithColor | C2D_AlignLeft, 5.0f, 5.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 git ? C2D_Color32(0,178,20,255) : C2D_Color32(204,2,112,255));
			C2D_DrawText(&other_Text[6], C2D_WithColor | C2D_AlignLeft, 5.0f, 25.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 genderMatch ? C2D_Color32(0,178,20,255) : C2D_Color32(204,2,112,255));
			C2D_DrawText(&other_Text[7], C2D_WithColor | C2D_AlignLeft, 5.0f, 45.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 raceMatch ? C2D_Color32(0,178,20,255) : C2D_Color32(204,2,112,255));

			C2D_DrawText(&other_Text[8], C2D_WithColor | C2D_AlignLeft, 5.0f, 65.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 eyesMatch ? C2D_Color32(0,178,20,255) : C2D_Color32(204,2,112,255));

			C2D_DrawText(&other_Text[9], C2D_WithColor | C2D_AlignLeft, 5.0f, 85.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 hairMatch ? C2D_Color32(0,178,20,255) : C2D_Color32(204,2,112,255));
						 
			C2D_DrawText(&other_Text[10], C2D_WithColor | C2D_AlignLeft, 5.0f, 105.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 heightMatch ? C2D_Color32(0,178,20,255) : C2D_Color32(204,2,112,255));

			C2D_DrawText(&other_Text[11], C2D_WithColor | C2D_AlignLeft, 5.0f, 125.0f, 1.0f, bounceScale * 0.9f, bounceScale * 0.9f,
						 yearMatch ? C2D_Color32(0,178,20,255) : C2D_Color32(204,2,112,255));
		}
	} else {
		C2D_DrawText(&other_Text[2], C2D_WithColor | C2D_AlignCenter , 198.0f, 56.0f, 1.0f, bounceScale * 1.1f, bounceScale * 1.1f, git ? C2D_Color32(0,0,0,255) : C2D_Color32(0,0,0,255));
		C2D_DrawText(&other_Text[2], C2D_WithColor | C2D_AlignCenter , 200.0f, 54.0f, 1.0f, bounceScale * 1.1f, bounceScale * 1.1f, git ? C2D_Color32(0,178,20,255) : C2D_Color32(204,2,112,255));
		C2D_DrawText(&other_Text[5], C2D_WithColor | C2D_AlignCenter , 198.0f, 89.0f, 1.0f, bounceScale * 1.1f, bounceScale * 1.1f, git ? C2D_Color32(0,0,0,255) : C2D_Color32(0,0,0,255));
		C2D_DrawText(&other_Text[5], C2D_WithColor | C2D_AlignCenter , 200.0f, 87.0f, 1.0f, bounceScale * 1.1f, bounceScale * 1.1f, git ? C2D_Color32(0,178,20,255) : C2D_Color32(204,2,112,255));
		C2D_DrawText(&other_Text[6], C2D_WithColor | C2D_AlignLeft, 10.0f, 10.0f, 1.0f, 0.5f, 0.5f, C2D_Color32(255,255,255,255));
	}
	float easeProgress = gitTimer / EASE_DURATION;
	float easedX = 10.0f + (200.0f - 10.0f) * powf(sinf(easeProgress * M_PI * 0.5f), 2.0f);


	#define START_X 50.0f
	#define TARGET_X (SCREEN_WIDTH / 2.0f)
	#define START_Y_DONE 160.0f
	#define START_Y_NOT_DONE 180.0f
	#define TARGET_Y_OFFSET 30.0f
	#define IMG_ORIG_SIZE 150.0f
	#define IMG_SCALE 0.5f
	#define IMG_SIZE (IMG_ORIG_SIZE * IMG_SCALE)

	float drawX, drawY;

	if (pressed_Once) {
		if (pfp_image.done) {
			float startY = START_Y_DONE;
			if (git && gitTimer > 0.0f) {
				float easeProgress = gitTimer / EASE_DURATION;
				if (easeProgress > 1.0f) easeProgress = 1.0f;

				float eased = powf(sinf(easeProgress * M_PI * 0.5f), 2.0f);
				float centerX = START_X + (TARGET_X - START_X) * eased;
				drawX = centerX - (IMG_SIZE / 2.0f);
				drawY = startY + (TARGET_Y_OFFSET * -1.0f) * eased;  // Moving up by TARGET_Y_OFFSET
			} else {
				// git == false OR gitTimer == 0, reset to start position
				drawX = START_X - (IMG_SIZE / 2.0f);
				drawY = startY;
			}

			// Draw with shadow
			C2D_ImageTint shadowTint;
			C2D_PlainImageTint(&shadowTint, C2D_Color32(0, 0, 0, 110), 1.0f);
			C2D_DrawImageAt(kuponkurwa, drawX - 4.0f, drawY, 1.0f, &shadowTint, IMG_SCALE, IMG_SCALE);
			C2D_DrawImageAt(kuponkurwa, drawX, drawY - 4.0f, 1.0f, NULL, IMG_SCALE, IMG_SCALE);

		} else { // !pfp_image.done && pressed_Once
			float startY = START_Y_NOT_DONE;
			if (git && gitTimer > 0.0f) {
				float easeProgress = gitTimer / EASE_DURATION;
				if (easeProgress > 1.0f) easeProgress = 1.0f;

				float eased = powf(sinf(easeProgress * M_PI * 0.5f), 2.0f);
				float centerX = START_X + (TARGET_X - START_X) * eased;
				drawX = centerX - (IMG_SIZE / 2.0f);
				drawY = startY + (TARGET_Y_OFFSET * -1.0f) * eased;
			} else {
				drawX = START_X - (IMG_SIZE / 2.0f);
				drawY = startY;
			}

			C2D_DrawText(&other_Text[3], C2D_WithColor | C2D_AlignLeft, drawX, drawY, 1.0f, 0.55f, 0.55f, C2D_Color32(255,255,255,255));
		}
	}
    // Draw fade to black overlay if fading started
    if (fadeStarted) {
		float fadeProgress = fadeTimer / FADE_DURATION;
		if (fadeProgress > 1.0f) fadeProgress = 1.0f;

		float fadeValue = 1.0f - fadeProgress; // goes from 1.0 to 0.0
		uint8_t alpha = (uint8_t)(fadeValue * 255.0f);

        C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(0, 0, 0, alpha));
    }
    C2D_TargetClear(bottom, C2D_Color32(18, 18, 18, 255));

    
    C2D_SceneBegin(bottom);
	u32 color = C2D_Color32(210, 210, 210, 40);  // Light gray, 40/255 alpha
	if (!endless) {
		if (lista.parsed && parse_daily_vtuber(daily_plvtdle.data)) {
			C2D_DrawRectangle(
				0, 0,           // x, y
				0.3f,           // z-depth
				SCREEN_WIDTH, SCREEN_HEIGHT,
				color, color, color, color  // Same color on all corners
			);
		}
	} else {
		if (lista.parsed && get_random_vtuber_id(all_vtubers.data)) {
			C2D_DrawRectangle(
				0, 0,           // x, y
				0.3f,           // z-depth
				SCREEN_WIDTH, SCREEN_HEIGHT,
				color, color, color, color  // Same color on all corners
			);
		}	
	}
    C2D_TextBufClear(MainMenu);  
	if (!endless) {
		if (daily_plvtdle.done) {
			if (lista.parsed && parse_daily_vtuber(daily_plvtdle.data)) {
				if (!git) {
					strona = choose / ITEMS_PER_PAGE;
					draw_names(&lista, MainMenu, 160.0f, names_Y - outHeight, 20.0f, strona, choose);
					C2D_DrawText(&other_Text[1], C2D_WithColor, 260.0f, 120.0f - outHeight, 1.0f, 0.9f, 0.9f, C2D_Color32(255,255,255,255));
				}
			} else {
				C2D_DrawText(&other_Text[3], C2D_WithColor | C2D_AlignCenter, 160.0f, 120.0f - outHeight, 1.0f, 0.9f, 0.9f, C2D_Color32(255,255,255,255));
			}
		} else {
			C2D_DrawText(&other_Text[3], C2D_WithColor | C2D_AlignCenter, 160.0f, 120.0f - outHeight, 1.0f, 0.9f, 0.9f, C2D_Color32(255,255,255,255));
		}
	} else {
		if (all_vtubers.done) {
			if (lista.parsed && get_random_vtuber_id(all_vtubers.data)) {
				if (!git) {
					strona = choose / ITEMS_PER_PAGE;
					draw_names(&lista, MainMenu, 160.0f, names_Y - outHeight, 20.0f, strona, choose); 
					C2D_DrawText(&other_Text[1], C2D_WithColor, 260.0f, 120.0f - outHeight, 1.0f, 0.9f, 0.9f, C2D_Color32(255,255,255,255));
				}
			} else {
				C2D_DrawText(&other_Text[3], C2D_WithColor | C2D_AlignCenter, 160.0f, 120.0f - outHeight, 1.0f, 0.9f, 0.9f, C2D_Color32(255,255,255,255));
			}
		} else {
			C2D_DrawText(&other_Text[3], C2D_WithColor | C2D_AlignCenter, 160.0f, 120.0f - outHeight, 1.0f, 0.9f, 0.9f, C2D_Color32(255,255,255,255));
		}
	}
    // Draw fade to black overlay if fading started
    if (fadeStarted) {
		float fadeProgress = fadeTimer / FADE_DURATION;
		if (fadeProgress > 1.0f) fadeProgress = 1.0f;

		float fadeValue = 1.0f - fadeProgress; // goes from 1.0 to 0.0
		uint8_t alpha = (uint8_t)(fadeValue * 255.0f);

        C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(0, 0, 0, alpha));
    }
}




void sceneMainMenuExit(void) {
    // Free menu resources
}
