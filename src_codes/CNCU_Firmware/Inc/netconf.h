#ifndef __NETCONF_H
#define __NETCONF_H

#ifdef __cplusplus
 extern "C" {
#endif
   
/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
//void LwIP_Init(void);
void LwIP_DHCP_task(void * pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* __NETCONF_H */
