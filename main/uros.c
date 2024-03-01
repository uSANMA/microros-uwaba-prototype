#include <uros_network_interfaces.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <std_msgs/msg/int32.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
#include <rmw_microros/rmw_microros.h>
#endif

#define RCCHECK(fn)                                                                      \
	{                                                                                    \
		rcl_ret_t temp_rc = fn;                                                          \
		if ((temp_rc != RCL_RET_OK))                                                     \
		{                                                                                \
			printf("Falha na linha %d: %d. Abortado.\n", __LINE__, (int)temp_rc); \
		}                                                                                \
	}
#define RCSOFTCHECK(fn)                                                                    \
	{                                                                                      \
		rcl_ret_t temp_rc = fn;                                                            \
		if ((temp_rc != RCL_RET_OK))                                                       \
		{                                                                                  \
			printf("Falha na linha %d: %d. Ignorado.\n", __LINE__, (int)temp_rc); \
		}                                                                                  \
	}

rcl_publisher_t publisher;
rcl_subscription_t subscriber;
std_msgs__msg__Int32 send_msg;
std_msgs__msg__Int32 recv_msg;

void timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
	RCLC_UNUSED(last_call_time);
	if (timer != NULL) {
		RCSOFTCHECK(rcl_publish(&publisher, &send_msg, NULL));
        ESP_LOGI("uROS_Topic","/encoder/%ln", &send_msg.data);
		send_msg.data++;
	}
}

void subscription_callback(const void * msgin) {
    //recebe mensagem de um pub
    const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *) msgin;
    int motor_cmd = msg->data;
    ESP_LOGI("uROS_Topic","/cmd/%d", motor_cmd);
    int32_t volta = motor_cmd * 2;

    //publica uma nova mensagem
    RCSOFTCHECK(rcl_publish(&publisher, &volta, NULL));
}

void micro_ros_task(void * arg) {

    //inicia struct
    rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;

    //define as opções de inicialização de acordo com default, menuconfig e allocador
    rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
    RCCHECK(rcl_init_options_init(&init_options, allocator));
    ESP_LOGI("uROS","Memoria alocada");

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
    rmw_init_options_t* rmw_options = rcl_init_options_get_rmw_init_options(&init_options);
    //descobre de forma automarica quem é o agente ativo do ros na rede
    //RCCHECK(rmw_uros_discover_agent(rmw_options));

    //podemos encontrar um agente colocando suas informações
    RCCHECK(rmw_uros_options_set_udp_address(CONFIG_MICRO_ROS_AGENT_IP, CONFIG_MICRO_ROS_AGENT_PORT, rmw_options));
    ESP_LOGI("uROS","Agente configurado");
#endif

    //criando init_options
    RCCHECK(rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator));
    ESP_LOGI("uROS","init_options configurado");

    //criando um node
    rcl_node_t node;
    RCCHECK(rclc_node_init_default(
        &node, 
        "uWABA", 
        "", 
        &support));
    ESP_LOGI("uROS","Node criado");

    //criando um publisher
    RCCHECK(rclc_publisher_init_default(
        &publisher, 
        &node, 
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), 
        "/motor/encoder"));
    ESP_LOGI("uROS","publisher criado");

    //criando um subscriber
    RCCHECK(rclc_subscription_init_default(
		&subscriber,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
		"/motor/cmd"));
    ESP_LOGI("uROS","subscriber criado");

    //Criando timer timer.
	rcl_timer_t timer;
	const unsigned int timer_timeout = 100;
	RCCHECK(rclc_timer_init_default(
		&timer,
		&support,
		RCL_MS_TO_NS(timer_timeout),
		timer_callback));

    //criando um executor
    rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();
	RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
    RCCHECK(rclc_executor_set_timeout(&executor, RCL_MS_TO_NS(1000)));

    //timer para o executor realizar o spin
    //somente é chamado quando um novo dado é criado ON_NEW_DATA; pode ser utilizado o allways
    RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &recv_msg, &subscription_callback, ON_NEW_DATA));
    RCCHECK(rclc_executor_add_timer(&executor, &timer));

    //rodar o executor
    rclc_executor_spin(&executor);
    ESP_LOGI("uROS","executor spin");

    //Limpa memoria
	//RCCHECK(rcl_subscription_fini(&subscriber, &node));
	RCCHECK(rcl_publisher_fini(&publisher, &node));
	RCCHECK(rcl_node_fini(&node));
    ESP_LOGI("uROS","memoria limpa");

    vTaskDelete(NULL);
}