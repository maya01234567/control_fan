/*
The MIT License (MIT)

Copyright (c) 2017 Eugene Zagidullin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_task.h"
#include "esp_log.h"

#include "blynk_iot.h"

#define BLYNK_MAGIC 0xf932d1fb
#define BLYNK_CHECK_MAGIC(c)                  \
	do                                        \
	{                                         \
		if ((c)->magic != BLYNK_MAGIC)        \
		{                                     \
			return BLYNK_ERR_NOT_INITIALIZED; \
		}                                     \
	} while (0)

#define DEFAULT_PING_INTERVAL 2000
#define DEFAULT_TIMEOUT 5000
#define DEFAULT_RECONNECT_DELAY 5000

#define QUEUE_SIZE 8
#define BLYNK_TASK_PRIO 1
#define BLYNK_TASK_STACK_SIZE (8 * 1024)

const char *tag = "blynk";

static const char default_server[] = "sgp1.blynk.cloud";
static const char default_port[] = "80";

static void parse_cmd(blynk_client_t *c, uint8_t d);
static void parse_id(blynk_client_t *c, uint8_t d);
static void parse_len(blynk_client_t *c, uint8_t d);
static void parse_payload(blynk_client_t *c, uint8_t d);
static void handle_message(blynk_client_t *c);
static int blynk_socketpair(int fds[2]);
static TickType_t get_timeout(const blynk_client_t *c);

typedef struct
{
	blynk_message_t message;
	TickType_t deadline;
	blynk_response_handler_t handler;
	void *data;
} blynk_ctl_t;

blynk_err_t blynk_init(blynk_client_t *c)
{
	memset(c, 0, sizeof(blynk_client_t));

	c->state.state = BLYNK_STATE_STOPPED;

	if ((c->state.mtx = xSemaphoreCreateMutex()) == 0)
	{
		return BLYNK_ERR_MEM;
	}

	if ((c->priv.ctl_queue = xQueueCreate(QUEUE_SIZE, sizeof(blynk_ctl_t))) == 0)
	{
		return BLYNK_ERR_MEM;
	}

	if (blynk_socketpair(c->priv.ctl) < 0)
	{
		return BLYNK_ERR_ERRNO;
	}

	int flags = fcntl(c->priv.ctl[0], F_GETFL, 0);
	fcntl(c->priv.ctl[0], F_SETFL, flags | O_NONBLOCK);

	flags = fcntl(c->priv.ctl[1], F_GETFL, 0);
	fcntl(c->priv.ctl[1], F_SETFL, flags | O_NONBLOCK);

	c->magic = BLYNK_MAGIC;

	return BLYNK_OK;
}

blynk_err_t blynk_set_options(blynk_client_t *c, const blynk_options_t *opt)
{
	BLYNK_CHECK_MAGIC(c); // kiểm tra c hợp lệ không

	if (!opt->token[0])
	{
		return BLYNK_ERR_INVALID_OPTION;
	}

	blynk_state_data_t *s = &c->state;

	xSemaphoreTake(s->mtx, portMAX_DELAY);
	s->opt = *opt;

	if (!s->opt.server[0])
	{
		snprintf(s->opt.server, sizeof(s->opt.server), "%s:%s", default_server, default_port);
	}
	else if (!memchr(s->opt.server, ':', sizeof(s->opt.server)))
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
		snprintf(s->opt.server, sizeof(s->opt.server), "%s:%s", opt->server, default_port);
#pragma GCC diagnostic pop
	}

	if (!s->opt.ping_interval)
	{
		s->opt.ping_interval = DEFAULT_PING_INTERVAL;
	}

	if (!s->opt.timeout)
	{
		s->opt.timeout = DEFAULT_TIMEOUT;
	}

	if (!s->opt.reconnect_delay)
	{
		s->opt.reconnect_delay = DEFAULT_RECONNECT_DELAY;
	}

	xSemaphoreGive(s->mtx);
	printf("%s\n", s->opt.server);
	printf("%s\n", s->opt.token);

	return BLYNK_OK;
}

/* All callback functions are called from Blynk client task */

blynk_err_t blynk_set_state_handler(blynk_client_t *c, blynk_state_handler_t handler, void *data)
{
	BLYNK_CHECK_MAGIC(c);

	xSemaphoreTake(c->state.mtx, portMAX_DELAY);
	c->state.evt_handler = handler;
	c->state.evt_handler_data = data;
	xSemaphoreGive(c->state.mtx);

	return BLYNK_OK;
}

/* All callback functions are called from Blynk client task */

blynk_err_t blynk_set_handler(blynk_client_t *c, const char *command, blynk_handler_t handler, void *data)
{
	BLYNK_CHECK_MAGIC(c);

	blynk_state_data_t *s = &c->state;
	xSemaphoreTake(s->mtx, portMAX_DELAY);

	int i;
	for (i = 0; i < BLYNK_MAX_HANDLERS; i++)
	{
		if (strncmp(s->handlers[i].command, command, sizeof(s->handlers[i].command)) == 0)
		{
			s->handlers[i].handler = handler;
			s->handlers[i].data = data;

			xSemaphoreGive(s->mtx);
			return BLYNK_OK;
		}
	}

	/* find first free slot */
	for (i = 0; i < BLYNK_MAX_HANDLERS; i++)
	{
		if (!s->handlers[i].command[0])
		{
			strlcpy(s->handlers[i].command, command, sizeof(s->handlers[i].command));
			s->handlers[i].handler = handler;
			s->handlers[i].data = data;

			xSemaphoreGive(s->mtx);
			return BLYNK_OK;
		}
	}

	xSemaphoreGive(s->mtx);
	return BLYNK_ERR_MEM;
}

blynk_err_t blynk_remove_handler(blynk_client_t *c, const char *command)
{
	BLYNK_CHECK_MAGIC(c);

	blynk_state_data_t *s = &c->state;
	xSemaphoreTake(s->mtx, portMAX_DELAY);

	int i;
	for (i = 0; i < BLYNK_MAX_HANDLERS; i++)
	{
		if (strncmp(s->handlers[i].command, command, sizeof(s->handlers[i].command)) == 0)
		{
			s->handlers[i].command[0] = 0;
			s->handlers[i].handler = NULL;
			s->handlers[i].data = NULL;
			break;
		}
	}
	xSemaphoreGive(s->mtx);

	return BLYNK_OK;
}

static uint16_t get_id(blynk_client_t *c, TickType_t deadline, blynk_response_handler_t handler, void *data)
{
	blynk_private_t *p = &c->priv;

	/* wrap id */
	if (p->id == (uint16_t)(-1))
	{
		p->id = 0;
		memset(p->awaiting, 9, sizeof(p->awaiting));
	}
	uint16_t id = p->id++;
	if (!handler)
	{
		return id;
	}

	int i;
	for (i = 0; i < BLYNK_MAX_AWAITING; i++) // chờ lấy id
	{
		if (!p->awaiting[i].id)
		{
			printf("set a waiting \n");
			p->awaiting[i].handler = handler;
			p->awaiting[i].id = id;
			p->awaiting[i].data = data;
			p->awaiting[i].deadline = deadline;
			printf("id %d\n", id);
			return id;
		}
	}

	return 0;
}

static size_t blynk_serialize(uint8_t *buf, size_t buf_sz, const blynk_message_t *msg)
{
	buf[0] = msg->command;			// 1 byte lenh
	buf[1] = (msg->id >> 8) & 0xff; // 2 byte id
	buf[2] = msg->id & 0xff;
	buf[3] = (msg->len >> 8) & 0xff; // 2byte do dai
	buf[4] = msg->len & 0xff;
	// printf("buff %s\n",(char*)buf);

	size_t msg_sz = 5;

	if (msg->len && msg->command != BLYNK_CMD_RESPONSE)
	{
		size_t pl_sz = msg->len;

		if (pl_sz > buf_sz - msg_sz)
		{
			pl_sz = buf_sz - msg_sz;
		}
		memcpy(buf + msg_sz, msg->payload, pl_sz);
		msg_sz += pl_sz;
		printf("msg_size %d\n", msg_sz);
	}

	return msg_sz;
}

static blynk_err_t blynk_send_internal(blynk_client_t *c,
									   uint8_t cmd,
									   uint16_t id,
									   uint16_t len,
									   uint8_t *payload,
									   blynk_response_handler_t handler,
									   void *data,
									   TickType_t wait)
{
	blynk_ctl_t ctl = {
		.message = {
			.command = cmd,
			.id = id,
			.len = len,
		},
		.deadline = handler ? get_timeout(c) + xTaskGetTickCount() : 0,
		.handler = handler,
		.data = data,
	};
	printf("data : %d \n", ctl.message.command);
	if (cmd != BLYNK_CMD_RESPONSE && len && payload)
	{
		printf("blynk different cmd response \n");
		if (len > sizeof(ctl.message.payload))
		{
			len = ctl.message.len = sizeof(ctl.message.payload);
		}
		printf("payload %s \n", ctl.message.payload);
		memcpy(ctl.message.payload, payload, len);
		printf("payload2 %s \n", ctl.message.payload);
	}

	if (!xQueueSend(c->priv.ctl_queue, &ctl, wait))
	{
		printf("queue send err \n");
		return BLYNK_ERR_MEM;
	}

	/* cancel select */
	uint8_t dummy = 0;
	printf("priv ctr %n \n", c->priv.ctl);
	if (write(c->priv.ctl[1], &dummy, 1) < 0) // ghi 0 vào phần thử ctr[1]
	{
		return BLYNK_ERR_ERRNO;
	}
	printf("priv ctr2  %n \n", c->priv.ctl);

	return BLYNK_OK;
}

static void parse_cmd(blynk_client_t *c, uint8_t d)
{
	printf("parse cmd  \n");
	blynk_private_t *p = &c->priv;
	p->message.command = d;
	p->cnt = 0;
	p->parse = parse_id;
}

static void parse_id(blynk_client_t *c, uint8_t d)
{
	blynk_private_t *p = &c->priv;
	p->message.id = (p->message.id << 8) | d;
	p->cnt++;

	if (p->cnt >= 2)
	{
		p->cnt = 0;
		p->parse = parse_len;
	}
}

static void parse_len(blynk_client_t *c, uint8_t d)
{
	blynk_private_t *p = &c->priv;
	p->message.len = (p->message.len << 8) | d;
	p->cnt++;

	if (p->cnt >= 2)
	{
		if (p->message.command != BLYNK_CMD_RESPONSE && p->message.len)
		{
			p->cnt = 0;
			p->parse = parse_payload;
		}
		else
		{
			handle_message(c);
			p->parse = parse_cmd;
		}
	}
}

static void parse_payload(blynk_client_t *c, uint8_t d)
{
	blynk_private_t *p = &c->priv;
	if (p->cnt < sizeof(p->message.payload))
	{
		p->message.payload[p->cnt] = d;
	}
	p->cnt++;

	if (p->cnt >= p->message.len)
	{
		if (p->cnt > sizeof(p->message.payload))
		{
			p->message.len = sizeof(p->message.payload);
		}
		handle_message(c);
		p->parse = parse_cmd;
	}
}

static void set_state(blynk_client_t *c, blynk_state_t state)
{
	blynk_state_evt_t ev = {
		.state = state,
	};

	xSemaphoreTake(c->state.mtx, portMAX_DELAY);

	c->state.state = state;
	blynk_state_handler_t handler = c->state.evt_handler;
	void *data = c->state.evt_handler_data;

	xSemaphoreGive(c->state.mtx);

	if (handler)
		handler(c, &ev, data);
}

blynk_state_t blynk_get_state(blynk_client_t *c)
{
	xSemaphoreTake(c->state.mtx, portMAX_DELAY);
	blynk_state_t state = c->state.state;
	xSemaphoreGive(c->state.mtx);

	return state;
}

static void set_disconnected(blynk_client_t *c, blynk_err_t reason, int code)
{
	blynk_state_evt_t ev = {
		.state = BLYNK_STATE_DISCONNECTED,
		.disconnected = {
			.reason = reason,
			.code = code,
		},
	};

	xSemaphoreTake(c->state.mtx, portMAX_DELAY);

	c->state.state = BLYNK_STATE_DISCONNECTED;
	blynk_state_handler_t handler = c->state.evt_handler;
	void *data = c->state.evt_handler_data;

	xSemaphoreGive(c->state.mtx);

	ESP_LOGW(tag, "disconnected, reason: %d, code: %d", reason, code);

	if (handler)
		handler(c, &ev, data);
}

static int parse_args(char *payload, int len, char **args, int sz)
{
	char *p = payload;
	int i = 0;

	while (i < sz && len)
	{
		args[i++] = p;

		while (len && *p)
		{
			p++;
			len--;
		}

		if (len)
		{
			p++;
			len--;
		}
		else
		{
			*p = 0;
		}
	}

	return i;
}

static void handle_message(blynk_client_t *c)
{
	printf("hanlder mess \n");
	blynk_state_data_t *s = &c->state;
	blynk_private_t *p = &c->priv;

	if (p->message.command == BLYNK_CMD_RESPONSE)
	{
		printf("blynk_respon \n");
		int i;
		for (i = 0; i < BLYNK_MAX_AWAITING; i++)
		{
			if (p->awaiting[i].id && p->awaiting[i].id == p->message.id)
			{
				if (p->awaiting[i].handler)
				{
					printf("call back handler \n");
					p->awaiting[i].handler(c, p->message.len, p->awaiting[i].data);//truyền tham số hàm auth
				}

				p->awaiting[i].id = 0;
			}
		}
	}
	else if (p->message.command == BLYNK_CMD_HARDWARE)
	{
		char *args[BLYNK_MAX_ARGS];
		int n = 0;
		if (p->message.len)
		{
			/* leave space for terminating \0 */
			int len = p->message.len < sizeof(p->message.payload) ? p->message.len : sizeof(p->message.payload) - 1;

			n = parse_args((char *)p->message.payload, len, args, BLYNK_MAX_ARGS);
		}

		if (n)
		{
			xSemaphoreTake(s->mtx, portMAX_DELAY);

			int i;
			for (i = 0; i < BLYNK_MAX_HANDLERS; i++)
			{
				if (strncmp(s->handlers[i].command, args[0], sizeof(s->handlers[i].command)) == 0 && s->handlers[i].handler)
				{

					blynk_handler_t handler = s->handlers[i].handler;
					void *data = s->handlers[i].data;

					xSemaphoreGive(s->mtx);

					handler(c, p->message.id, args[0], n - 1, args + 1, data);
					return;
				}
			}

			xSemaphoreGive(s->mtx);

			blynk_err_t ret = blynk_send_internal(c,
												  BLYNK_CMD_RESPONSE,
												  p->message.id,
												  BLYNK_STATUS_ILLEGAL_COMMAND,
												  NULL, NULL, NULL, 0);
			if (ret != BLYNK_OK)
			{
				set_disconnected(c, ret, ret == BLYNK_ERR_ERRNO ? errno : 0);
			}
		}
	}
}

static int blynk_socketpair(int fds[2])
{
	int new_fds[2];
	if ((new_fds[0] = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		goto fail_0;
	}

	struct sockaddr addr;
	memset(&addr, 0, sizeof(addr));
	((struct sockaddr_in *)&addr)->sin_family = AF_INET;
	((struct sockaddr_in *)&addr)->sin_addr.s_addr = htonl(0x7f000001);
	socklen_t len = sizeof(struct sockaddr_in);

	if (bind(new_fds[0], &addr, len) < 0)
	{
		goto fail_1;
	}

	len = sizeof(addr);
	if (getsockname(new_fds[0], &addr, &len) < 0)
	{
		goto fail_1;
	}

	if ((new_fds[1] = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		goto fail_1;
	}

	if (connect(new_fds[1], &addr, len) < 0)
	{
		goto fail_2;
	}

	len = sizeof(addr);
	if (getsockname(new_fds[1], &addr, &len) < 0)
	{
		goto fail_2;
	}

	if (connect(new_fds[0], &addr, len) < 0)
	{
		goto fail_2;
	}

	fds[0] = new_fds[0];
	fds[1] = new_fds[1];

	return 0;

fail_2:
	close(new_fds[1]);

fail_1:
	close(new_fds[0]);

fail_0:
	return -1;
}

static void auth_cb(blynk_client_t *c, uint16_t status, void *data)
{
	ESP_LOGI(tag, "auth: %d", status);

	if (status != BLYNK_STATUS_SUCCESS)
	{
		if (status == BLYNK_STATUS_RESPONSE_TIMEOUT)
		{
			ESP_LOGI(tag, "time_out: %u", status);
			set_disconnected(c, BLYNK_ERR_TIMEOUT, 0);
		}
		else
		{
			ESP_LOGI(tag, "!= BLYNK_STATUS_SUCCESS not time out \n");
			set_disconnected(c, BLYNK_ERR_STATUS, status);
		}
	}
	else
	{
		set_state(c, BLYNK_STATE_AUTHENTICATED);
	}
}

static void ping_cb(blynk_client_t *c, uint16_t status, void *data)
{
	if (status != BLYNK_STATUS_SUCCESS)
	{
		if (status == BLYNK_STATUS_RESPONSE_TIMEOUT)
		{
			set_disconnected(c, BLYNK_ERR_TIMEOUT, 0);
		}
		else
		{
			set_disconnected(c, BLYNK_ERR_STATUS, status);
		}
	}
}

static inline bool timerExpired(TickType_t deadline, TickType_t now)
{
	/* Avoid direct comparison of unsigned numbers */
	return (int32_t)((uint32_t)deadline - (uint32_t)now) <= 0;
}

static bool get_select_timeout(blynk_client_t *c, TickType_t *timeout)
{
	/* find time until next event */
	blynk_private_t *p = &c->priv;

	TickType_t now = xTaskGetTickCount();
	TickType_t min = 0;
	int i;
	bool ret = false;
	for (i = 0; i < BLYNK_MAX_AWAITING; i++)
	{
		if (!p->awaiting[i].id || !p->awaiting[i].deadline)
		{
			continue;
		}

		if (timerExpired(p->awaiting[i].deadline, now))
		{
			/* deadline already expired, so zero timeout */
			*timeout = 0;
			return true;
		}

		TickType_t diff = p->awaiting[i].deadline - now;
		if (!ret || diff < min)
		{
			min = diff;
			ret = true;
		}
	}

	if (p->ping_deadline)
	{
		if (timerExpired(p->ping_deadline, now))
		{
			/* deadline already expired, so zero timeout */
			*timeout = 0;
			return true;
		}

		TickType_t diff = p->ping_deadline - now;
		if (!ret || diff < min)
		{
			min = diff;
			ret = true;
		}
	}

	if (ret)
	{
		*timeout = min;
	}

	return ret;
}

static void advance_ping(blynk_client_t *c)
{
	xSemaphoreTake(c->state.mtx, portMAX_DELAY);
	unsigned long ping_interval = c->state.opt.ping_interval;
	xSemaphoreGive(c->state.mtx);

	c->priv.ping_deadline = xTaskGetTickCount() + (TickType_t)ping_interval / portTICK_RATE_MS;
}

static TickType_t get_timeout(const blynk_client_t *c)
{
	xSemaphoreTake(c->state.mtx, portMAX_DELAY);
	unsigned long timeout = c->state.opt.timeout;
	xSemaphoreGive(c->state.mtx);

	return (TickType_t)timeout / portTICK_RATE_MS;
}

static blynk_err_t handle_timers(blynk_client_t *c)
{
	blynk_private_t *p = &c->priv;

	TickType_t now = xTaskGetTickCount();

	int i;
	for (i = 0; i < BLYNK_MAX_AWAITING; i++)
	{
		if (!p->awaiting[i].id || !p->awaiting[i].deadline)
		{
			continue;
		}

		if (timerExpired(p->awaiting[i].deadline, now))
		{
			/* deadline expired */
			if (p->awaiting[i].handler)
			{
				p->awaiting[i].handler(c, BLYNK_STATUS_RESPONSE_TIMEOUT, p->awaiting[i].data);
			}

			p->awaiting[i].id = 0;
		}
	}

	if (p->ping_deadline && timerExpired(p->ping_deadline, now))
	{
		advance_ping(c);
		return blynk_send_internal(c, BLYNK_CMD_PING, 0, 0, NULL, ping_cb, NULL, 0);
	}

	return BLYNK_OK;
}

static blynk_err_t blynk_loop(blynk_client_t *c)
{
	printf("token : %s\n", c->state.opt.token);
	printf("sever : %s\n", c->state.opt.server);
	ESP_LOGI(tag, "loop...");
	blynk_state_data_t *s = &c->state;
	blynk_private_t *p = &c->priv;

	xSemaphoreTake(s->mtx, portMAX_DELAY);
	blynk_options_t opt = s->opt;
	xSemaphoreGive(s->mtx);

	char hostname[128];
	strlcpy(hostname, opt.server, sizeof(hostname));

	char *servname = strchr(hostname, ':'); // tìm ký tự : trong url
	if (!servname)
	{
		return BLYNK_ERR_INVALID_OPTION;
	}
	*(servname++) = 0; // gán giá trị mà vị trí con trỏ trỏ đến = 0 sau đó tăng con tro len 1

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;

	struct addrinfo *result;
	int res;
	printf("token1 : %s\n", hostname);
	printf("sever1 : %s\n", servname);
	if ((res = getaddrinfo(hostname, servname, &hints, &result)) != 0)
	{
		printf("err\n");
		set_disconnected(c, BLYNK_ERR_GAI, res);
		goto fail_0;
	}
	int fd = -1;
	struct addrinfo *r;
	printf("family : %d\n", result->ai_family);
	printf("sockettype : %d\n", result->ai_socktype);
	printf("protocol : %d\n", result->ai_protocol);
	for (r = result; r != NULL; r = r->ai_next)
	{
		printf("family : %d\n", r->ai_family);
		printf("sockettype : %d\n", r->ai_socktype);
		printf("protocol : %d\n", r->ai_protocol);
		fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
		if (fd < 0) //,0 mở soket thất bại
		{
			printf("open soket err \n");
			continue;
		}

		if (connect(fd, r->ai_addr, r->ai_addrlen) < 0)
		{
			printf("connect soket err \n");
			close(fd);
			fd = -1;
			continue;
		}

		break;
	}
	freeaddrinfo(result);
	printf("finish soket : %d\n", fd);
	if (fd < 0)
	{
		set_disconnected(c, BLYNK_ERR_ERRNO, errno);
		goto fail_0;
	}

	int flags = fcntl(fd, F_GETFL, 0);
	printf("finish fget : %d\n", flags);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	set_state(c, BLYNK_STATE_CONNECTED);

	/* reset state */
	memset(&p->awaiting, 0, sizeof(p->awaiting));
	xQueueReset(c->priv.ctl_queue);
	p->parse = parse_cmd;
	p->id = 1;
	p->buf_total = 0;

	/* set ping timer */
	advance_ping(c);

	/* login */
	blynk_err_t ret;
	// gửi 1 lệnh điều khiển vào hanfgd đợi c->priv.ctl_queue
	if ((ret = blynk_send_internal(c, BLYNK_CMD_LOGIN, 0, strlen(opt.token),
								   (uint8_t *)opt.token, auth_cb, NULL, 0)) != BLYNK_OK)
	{
		printf("err\n");
		set_disconnected(c, ret, ret == BLYNK_ERR_ERRNO ? errno : 0);
		goto fail_1;
	}

	/* read loop */
	for (;;)
	{
		fd_set rdset;
		fd_set wrset;

		FD_ZERO(&rdset); // Xóa tất cả các file descriptor khỏi tập hợp theo dõi sự kiện đọc.
		FD_ZERO(&wrset); // Xóa tất cả các file descriptor khỏi tập hợp theo dõi sự kiện ghi.

		FD_SET(fd, &rdset);		   // Thêm file descriptor fd vào tập hợp theo dõi sự kiện đọc.
		FD_SET(p->ctl[0], &rdset); // Thêm file descriptor p->ctl[0] (có vẻ là một socket kiểm soát) vào tập hợp theo dõi sự kiện đọc.

		blynk_ctl_t ctl;
		if (p->buf_total) // nếu có dữ liệu cần gửi thù thực hiện trong if
		{
			/* partial write happened before */
			FD_SET(fd, &wrset); // thêm file descriptor fd vào tập hợp theo dõi sự kiện ghi (FD_SET(fd, &wrset)).
		}
		else if (xQueueReceive(c->priv.ctl_queue, &ctl, 0)) // lấy 1 dữ liệu từ hàng đợi c->priv.ctl_queue lưu vào ctl
		{
			if (!ctl.message.id) // nếu id là 0(lệnh đăng nhập) thì thực hiện ở trong
			{
				/* allocate id (chỉ định id) */
				printf("ctl.deadline %u\n", ctl.deadline);
				printf("mess %s\n", (char *)ctl.message.payload);
				uint16_t id = get_id(c, ctl.deadline, ctl.handler, ctl.data); // gán id cho 1 hàng đợi lệnh p->awaiting

				if (!id)
				{
					printf("not data\n");
					set_disconnected(c, BLYNK_ERR_MEM, 0);
					goto fail_1;
				}

				ctl.message.id = id;
			}
			p->buf_total = blynk_serialize(p->wr_buf, sizeof(p->wr_buf), &ctl.message); // ghép data vào msg và trả về tổng độ dài msg
			p->buf_sent = 0;
			ESP_LOGI(tag, "msg %s \n", (char *)(p->wr_buf + 5));

			FD_SET(fd, &wrset);
		}

		TickType_t timeout;
		bool timeout_set = get_select_timeout(c, &timeout);
		struct timeval tv;

		if (timeout_set)
		{
			tv.tv_sec = (time_t)timeout * portTICK_RATE_MS / 1000;
			tv.tv_usec = (((time_t)timeout * portTICK_RATE_MS) % 1000) * 1000;
		}
		// hàm select kiểm tra xem có file đọc ghi nào khôgn
		int nfds = select(FD_SETSIZE, &rdset, &wrset, NULL, timeout_set ? &tv : NULL);
		printf("nfds:%d\n", nfds);
		if (nfds < 0)
		{
			set_disconnected(c, BLYNK_ERR_ERRNO, errno);
			goto fail_1;
		}

		/* handle timers */
		if ((ret = handle_timers(c)) != BLYNK_OK)
		{
			set_disconnected(c, ret, ret == BLYNK_ERR_ERRNO ? errno : 0);
			goto fail_1;
		}
		printf("s->state:%d\n", s->state);
		if (s->state == BLYNK_STATE_DISCONNECTED)
		{
			goto fail_1;
		}

		if (!nfds)
			continue;

		if (FD_ISSET(p->ctl[0], &rdset))
		{
			/* Control socket, do nothing */

			uint8_t dummy;
			int rd = read(p->ctl[0], &dummy, 1);

			if (rd < 0 && errno != EAGAIN)
			{
				set_disconnected(c, BLYNK_ERR_ERRNO, errno);
				goto fail_1;
			}
		}

		if (FD_ISSET(fd, &rdset))
		{
			int rd = read(fd, p->rd_buf, sizeof(p->rd_buf));

			if (rd < 0)
			{
				if (errno != EAGAIN)
				{
					set_disconnected(c, BLYNK_ERR_ERRNO, errno);
					goto fail_1;
				}
			}
			else if (!rd)
			{
				set_disconnected(c, BLYNK_ERR_CLOSED, 0);
				goto fail_1;
			}
			else
			{
				uint8_t *d = p->rd_buf;
				while (rd--)
				{
					p->parse(c, *(d++));

					if (s->state == BLYNK_STATE_DISCONNECTED)
					{
						goto fail_1;
					}
				}
			}
		}

		if (p->buf_total && FD_ISSET(fd, &wrset))
		{
			/* Send outbound message */
			//int wr = write(fd, p->wr_buf + p->buf_sent, p->buf_total - p->buf_sent);

			int wr = write(fd,"https://sgp1.blynk.cloud/external/api/update?token=0mUb3DKviCER9iOKVBNGNMxaxkC6dKbu&v1=150",92);
			printf("write %d\n",wr);

			if (wr < 0)
			{
				printf("write err\n");
				if (errno != EAGAIN)
				{
					set_disconnected(c, BLYNK_ERR_ERRNO, errno);
					goto fail_1;
				}
			}
			else
			{
				printf("write done\n");
				p->buf_sent += wr;
				if (p->buf_sent >= p->buf_total)
				{
					printf("write done 2\n");
					p->buf_total = 0;
				}
			}
		}
	}

fail_1:
	close(fd);

fail_0:
	return BLYNK_OK;
}

static void blynk_task(void *arg)
{
	blynk_client_t *c = (blynk_client_t *)arg;

	while (blynk_loop(c) == BLYNK_OK)
	{
		ESP_LOGI(tag, "reconnect...");
		xSemaphoreTake(c->state.mtx, portMAX_DELAY);
		unsigned int delay = c->state.opt.reconnect_delay;
		xSemaphoreGive(c->state.mtx);

		vTaskDelay(delay / portTICK_RATE_MS);
	}

	ESP_LOGE(tag, "terminating...");
	vTaskDelete(NULL);
}

blynk_err_t blynk_start(blynk_client_t *c)
{
	printf("data9: %s \n", c->state.opt.token);
	xSemaphoreTake(c->state.mtx, portMAX_DELAY);
	if (c->state.state != BLYNK_STATE_STOPPED)
	{
		printf("blynk stop \n");
		xSemaphoreGive(c->state.mtx);
		return BLYNK_ERR_RUNNING;
	}

	if (!xTaskCreate(blynk_task, "BlynkTask", BLYNK_TASK_STACK_SIZE, c, BLYNK_TASK_PRIO, &c->state.task))
	{
		printf("blynk task init err \n");
		xSemaphoreGive(c->state.mtx);
		return BLYNK_ERR_MEM;
	}

	c->state.state = BLYNK_STATE_DISCONNECTED;

	blynk_state_handler_t handler = c->state.evt_handler;
	void *data = c->state.evt_handler_data;

	xSemaphoreGive(c->state.mtx);

	blynk_state_evt_t ev = {
		.state = BLYNK_STATE_DISCONNECTED,
		.disconnected = {
			.reason = BLYNK_OK,
		},
	};

	if (handler) // nếu hanlder khác NULL thì thực hiện
		handler(c, &ev, data);
	return BLYNK_OK;
}

/*
	Format syntax is derived from Python struct format
	c:		char
	b:		signed char	integer
	B:		unsigned char
	?:		bool
	h:		short
	H:		unsigned short
	i:		int
	I:		unsigned int
	l:		long
	L:		unsigned long
	q:		long long
	Q:		unsigned long long
	f:		float
	d:		double
	s,p:	char*

	All callback functions are called from Blynk client task
*/

blynk_err_t blynk_send_with_callback_v(blynk_client_t *c,
									   uint8_t cmd,
									   blynk_response_handler_t handler,
									   void *data,
									   TickType_t wait, const char *fmt, va_list ap)
{

	BLYNK_CHECK_MAGIC(c);

	if (cmd == BLYNK_CMD_RESPONSE)
	{
		return BLYNK_ERR_INVALID_OPTION;
	}

	switch (blynk_get_state(c))
	{
	case BLYNK_STATE_STOPPED:
	case BLYNK_STATE_DISCONNECTED:
		return BLYNK_ERR_NOT_CONNECTED;

	case BLYNK_STATE_CONNECTED:
		return BLYNK_ERR_NOT_AUTHENTICATED;

	default:
		break;
	}

	char payload[BLYNK_MAX_PAYLOAD_LEN];
	uint16_t len = 0;
	char *p = payload;

	for (; *fmt && len < sizeof(payload); fmt++)
	{
		char buf[32];
		const char *arg;

		if (*fmt == 's' || *fmt == 'p')
		{
			arg = va_arg(ap, const char *);
		}
		else
		{
			switch (*fmt)
			{
			case 'c':
			case 'b':
			case 'B':
				snprintf(buf, sizeof(buf), "%c", va_arg(ap, int));
				break;

			case '?':
				snprintf(buf, sizeof(buf), "%s", (bool)va_arg(ap, int) ? "true" : "false");
				break;

			case 'h':
			case 'H':
			case 'i':
				snprintf(buf, sizeof(buf), "%d", va_arg(ap, int));
				break;

			case 'I':
				snprintf(buf, sizeof(buf), "%u", va_arg(ap, unsigned int));
				break;

			case 'l':
				snprintf(buf, sizeof(buf), "%ld", va_arg(ap, long));
				break;

			case 'L':
				snprintf(buf, sizeof(buf), "%lu", va_arg(ap, unsigned long));
				break;

			case 'q':
				snprintf(buf, sizeof(buf), "%lld", va_arg(ap, long long));
				break;

			case 'Q':
				snprintf(buf, sizeof(buf), "%llu", va_arg(ap, unsigned long long));
				break;

			case 'f':
			case 'd':
				snprintf(buf, sizeof(buf), "%.7f", va_arg(ap, double));
				break;

			default:
				continue;
			}
			arg = buf;
		}

		while (*arg && len < sizeof(payload))
		{
			*(p++) = *(arg++);
			len++;
		}

		if (len < sizeof(payload) && *(fmt + 1))
		{
			*(p++) = 0;
			len++;
		}
	}

	return blynk_send_internal(c, cmd, 0, len, len ? (uint8_t *)payload : NULL, handler, data, wait);
}

blynk_err_t blynk_send_with_callback(blynk_client_t *c,
									 uint8_t cmd,
									 blynk_response_handler_t handler,
									 void *data,
									 TickType_t wait, const char *fmt, ...)
{

	va_list ap;
	va_start(ap, fmt);
	blynk_err_t ret = blynk_send_with_callback_v(c, cmd, handler, data, wait, fmt, ap);
	va_end(ap);
	return ret;
}

blynk_err_t blynk_send(blynk_client_t *c, uint8_t cmd, TickType_t wait, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	blynk_err_t ret = blynk_send_with_callback_v(c, cmd, NULL, NULL, wait, fmt, ap);
	va_end(ap);
	return ret;
}

blynk_err_t blynk_send_response(blynk_client_t *c, uint16_t id, uint16_t status, TickType_t wait)
{
	BLYNK_CHECK_MAGIC(c);

	return blynk_send_internal(c, BLYNK_CMD_RESPONSE, id, status, NULL, NULL, NULL, wait);
}