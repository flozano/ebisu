#include <string.h>

#include "kii_def.h"
#include "kii_meta.h"

static char mHost[KII_HOST_SIZE+1];
static char mAppID[KII_APPID_SIZE+1];
static char mAppKey[KII_APPKEY_SIZE+1];
char g_netBuf[KII_NETBUF_SIZE];


int kii_init(char *site, char *appID, char *appKey)
{
    if ((strlen(site) != KII_SITE_SIZE) || (strlen(appID) != KII_APPID_SIZE) || (strlen(appKey) != KII_APPKEY_SIZE))
    {
        return -1;
    }
    else
    {
        if (strcmp(site, "CN") == 0)
        {
            strcpy(mHost, "api-cn2.kii.com");
        }
	 else if (strcmp(site, "JP") == 0)
	 {
            strcpy(mHost, "api-jp.kii.com");
	 }
	 else if (strcmp(site, "US") == 0)
	 {
            strcpy(mHost, "api.kii.com");
	 }
	 else if (strcmp(site, "SG") == 0)
	 {
            strcpy(mHost, "api-sg.kii.com");
	 }
	 else
	 {
	     return -1;
	 }
	 
	strcpy(mAppID, appID);
	strcpy(mAppKey, appKey);

	kii_meta_init();
	
	return 0;
    }
}

char *kii_getHost(void)
{
    return mHost;
}

char *kii_getAppID(void)
{
    return mAppID;
}

char * kii_getAppKey(void)
{
    return mAppKey;
}

