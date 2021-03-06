#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "stm_log.h"
#include "software_resolver.h"

#define SOFTWARE_RESOLVER_INIT_ERR_STR				"software resolver init error"
#define SOFTWARE_RESOLVER_SET_COUNTER_MODE_ERR_STR	"software resolver set counter mode error"
#define SOFTWARE_RESOLVER_GET_VALUE_ERR_STR			"software resolver get counter value error"
#define SOFTWARE_RESOLVER_SET_VALUE_ERR_STR			"software resolver set counter value error"
#define SOFTWARE_RESOLVER_START_ERR_STR				"software resolver start error"
#define SOFTWARE_RESOLVER_STOP_ERR_STR				"software resolver stop error"

#define mutex_lock(x)       						while (xSemaphoreTake(x, portMAX_DELAY) != pdPASS)
#define mutex_unlock(x)     						xSemaphoreGive(x)
#define mutex_create()      						xSemaphoreCreateMutex()
#define mutex_destroy(x)    						vQueueDelete(x)

static const char* SOFTWARE_RESOLVER_TAG = "SOFTWARE RESOLVER";
#define SOFTWARE_RESOLVER_CHECK(a, str, action)  if(!(a)) {                                            \
        STM_LOGE(SOFTWARE_RESOLVER_TAG,"%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str);     \
        action;                                                                       		\
        }

typedef struct software_resolver {
	timer_num_t 			timer_num;				/*!< Timer num */
	timer_pins_pack_t		timer_pins_pack;		/*!< Timer pins pack */
	uint32_t 				max_reload;				/*!< Max reload value */
	timer_counter_mode_t	counter_mode;			/*!< Counter mode */
	SemaphoreHandle_t   	lock;                   /*!< Software resolver mutex */
} software_resolver_t;

static void _software_resolver_cleanup(software_resolver_handle_t handle) {
	free(handle);
}

software_resolver_handle_t software_resolver_config(software_resolver_config_t *config)
{
	/* Check input conditions */
	SOFTWARE_RESOLVER_CHECK(config, SOFTWARE_RESOLVER_INIT_ERR_STR, return NULL);

	/* Allocate memory for handle structure */
	software_resolver_handle_t handle = calloc(1, sizeof(software_resolver_t));
	SOFTWARE_RESOLVER_CHECK(handle, SOFTWARE_RESOLVER_INIT_ERR_STR, return NULL);

	/* Configure ETR */
	etr_cfg_t software_resolver_cfg;
	software_resolver_cfg.timer_num = config->timer_num;
	software_resolver_cfg.timer_pins_pack = config->timer_pins_pack;
	software_resolver_cfg.max_reload = config->max_reload;
	software_resolver_cfg.counter_mode = config->counter_mode;
	SOFTWARE_RESOLVER_CHECK(!etr_config(&software_resolver_cfg), SOFTWARE_RESOLVER_INIT_ERR_STR, {_software_resolver_cleanup(handle); return NULL;});

	/* Update handle structure */
	handle->timer_num = config->timer_num;
	handle->timer_pins_pack = config->timer_pins_pack;
	handle->counter_mode = config->counter_mode;
	handle->max_reload = config->max_reload;
	handle->lock = mutex_create();

	return handle;
}

stm_err_t software_resolver_start(software_resolver_handle_t handle)
{
	SOFTWARE_RESOLVER_CHECK(handle, SOFTWARE_RESOLVER_START_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);

	int ret;
	ret = etr_start(handle->timer_num);
	if (ret) {
		STM_LOGE(SOFTWARE_RESOLVER_TAG, SOFTWARE_RESOLVER_START_ERR_STR);
		mutex_unlock(handle->lock);
		return STM_FAIL;
	}

	mutex_unlock(handle->lock);
	return STM_OK;
}

stm_err_t software_resolver_stop(software_resolver_handle_t handle)
{
	SOFTWARE_RESOLVER_CHECK(handle, SOFTWARE_RESOLVER_STOP_ERR_STR, return STM_ERR_INVALID_ARG);

	mutex_lock(handle->lock);

	int ret;
	ret = etr_stop(handle->timer_num);
	if (ret) {
		STM_LOGE(SOFTWARE_RESOLVER_TAG, SOFTWARE_RESOLVER_STOP_ERR_STR);
		mutex_unlock(handle->lock);
		return STM_FAIL;
	}

	mutex_unlock(handle->lock);
	return STM_OK;
}

stm_err_t software_resolver_get_value(software_resolver_handle_t handle, uint32_t *value)
{
	SOFTWARE_RESOLVER_CHECK(handle, SOFTWARE_RESOLVER_GET_VALUE_ERR_STR, return STM_ERR_INVALID_ARG);
	SOFTWARE_RESOLVER_CHECK(value, SOFTWARE_RESOLVER_GET_VALUE_ERR_STR, return STM_ERR_INVALID_ARG);
	
	mutex_lock(handle->lock);

	int ret;
	ret = etr_get_value(handle->timer_num, value);
	if (ret) {
		STM_LOGE(SOFTWARE_RESOLVER_TAG, SOFTWARE_RESOLVER_GET_VALUE_ERR_STR);
		mutex_unlock(handle->lock);
		return STM_FAIL;
	}

	mutex_unlock(handle->lock);
	return STM_OK;
}

stm_err_t software_resolver_set_value(software_resolver_handle_t handle, uint32_t value)
{
	SOFTWARE_RESOLVER_CHECK(handle, SOFTWARE_RESOLVER_SET_VALUE_ERR_STR, return STM_ERR_INVALID_ARG);
	
	mutex_lock(handle->lock);

	int ret;
	ret = etr_set_value(handle->timer_num, value);
	if (ret) {
		STM_LOGE(SOFTWARE_RESOLVER_TAG, SOFTWARE_RESOLVER_SET_VALUE_ERR_STR);
		mutex_unlock(handle->lock);
		return STM_FAIL;
	}

	mutex_unlock(handle->lock);
	return STM_OK;
}

stm_err_t software_resolver_set_mode(software_resolver_handle_t handle, timer_counter_mode_t counter_mode)
{
	SOFTWARE_RESOLVER_CHECK(handle, SOFTWARE_RESOLVER_SET_COUNTER_MODE_ERR_STR, return STM_ERR_INVALID_ARG);
	
	mutex_lock(handle->lock);

	int ret;
	ret = etr_set_mode(handle->timer_num, counter_mode);
	if (ret) {
		STM_LOGE(SOFTWARE_RESOLVER_TAG, SOFTWARE_RESOLVER_SET_COUNTER_MODE_ERR_STR);
		mutex_unlock(handle->lock);
		return STM_FAIL;
	}

	mutex_unlock(handle->lock);
	return STM_OK;
}

void software_resolver_destroy(software_resolver_handle_t handle)
{
	_software_resolver_cleanup(handle);
}