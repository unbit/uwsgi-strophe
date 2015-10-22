#include <uwsgi.h>
extern struct uwsgi_server uwsgi;

#include <strophe.h>

static void strophe_init() {
	static int strophe_initialized = 0;
	if (strophe_initialized) return;
	xmpp_initialize();
	strophe_initialized = 1;
}


static xmpp_ctx_t *strophe_ctx() {
	//xmpp_log_t *log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);
	//return xmpp_ctx_new(NULL, log);
	return xmpp_ctx_new(NULL, NULL);
}

static void strophe_send(xmpp_ctx_t *ctx, xmpp_conn_t *conn, char *to, char *msg, size_t len) {
        xmpp_stanza_t *message = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(message, "message");
        xmpp_stanza_set_type(message, "chat");
        xmpp_stanza_set_attribute(message, "to", to);

        xmpp_stanza_t *body = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(body, "body");

        xmpp_stanza_t *text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text_with_size(text, msg, len);
        xmpp_stanza_add_child(body, text);
        xmpp_stanza_add_child(message, body);

        xmpp_send(conn, message);
        xmpp_stanza_release(text);
        xmpp_stanza_release(body);
        xmpp_stanza_release(message);
}


static int strophe_alarm_handler_messages(xmpp_conn_t * const conn, void * const userdata) {
	struct uwsgi_thread *ut = (struct uwsgi_thread *)userdata;
        xmpp_ctx_t *ctx = (xmpp_ctx_t *) ut->data;

	for(;;) {
		ssize_t rlen = read(ut->pipe[1], ut->buf, uwsgi.log_master_bufsize);
		if (rlen < 0) {
			if (uwsgi_is_again()) break;
			xmpp_stop(ctx);
			ut->custom1 = 0;
			break;
		}
		if (rlen == 0) {
			xmpp_stop(ctx);
			ut->custom1 = 0;
			break;
		}
		char *dst = uwsgi_str((char *) ut->custom0);
		char *p, *c = NULL;
		uwsgi_foreach_token(dst, ",", p, c) {
			strophe_send(ctx, conn, p, ut->buf, rlen);
		}
		free(dst);
	}

	return 1;
}

static void strophe_alarm_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
                  const int error, xmpp_stream_error_t * const stream_error,
                  void * const userdata) {

	struct uwsgi_thread *ut = (struct uwsgi_thread *)userdata;
	xmpp_ctx_t *ctx = (xmpp_ctx_t *) ut->data;

	if (status == XMPP_CONN_CONNECT) {
		// send presence
		xmpp_stanza_t *pres = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(pres, "presence");
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);
		uwsgi_log("[uwsgi-strophe] %s CONNECTED\n", xmpp_conn_get_jid(conn));
	}
	else {
		xmpp_stop(ctx);
		ut->custom1 = 0;
		uwsgi_log("[uwsgi-strophe] %s DISCONNECTED\n", xmpp_conn_get_jid(conn));
	}	
}

static void alarm_strophe_loop(struct uwsgi_thread *ut) {
	size_t rlen = 0;
	char **argv = uwsgi_split_quoted(ut->data, strlen(ut->data), " \t", &rlen);
	if (rlen < 3) {
		uwsgi_log("invalid strophe syntax, must be <jid> password> <to[,to,to,...]>\n");
		exit(1);
	}

	ut->buf = (char *) uwsgi_malloc(uwsgi.log_master_bufsize);

	for(;;) {
		xmpp_ctx_t *ctx = strophe_ctx();
		ut->data = ctx;
		ut->custom0 = (uint64_t) argv[2];
		xmpp_conn_t *conn = xmpp_conn_new(ctx);
		xmpp_conn_set_jid(conn, argv[0]);
		xmpp_conn_set_pass(conn, argv[1]);

		// heck for alarms every second
		xmpp_timed_handler_add(conn, strophe_alarm_handler_messages, 1000, ut);
		xmpp_connect_client(conn, NULL, 0, strophe_alarm_handler, ut);

		ut->custom1 = 1;
		while(ut->custom1) {
			xmpp_run_once(ctx, 1000);
		}

		xmpp_conn_release(conn);
		xmpp_ctx_free(ctx);

		uwsgi_log("[uwsgi-strophe] %s RECONNECTING ...\n", argv[0]);
		sleep(3);
	}

}

static void alarm_strophe_init(struct uwsgi_alarm_instance *uai) {
	strophe_init();
	struct uwsgi_thread *ut = uwsgi_thread_new(alarm_strophe_loop);
        if (!ut) return;
        uai->data_ptr = ut;
        ut->data = uai->arg;
}

static void alarm_strophe_func(struct uwsgi_alarm_instance *uai, char *msg, size_t len) {
	struct uwsgi_thread *ut = (struct uwsgi_thread *) uai->data_ptr;
        ut->rlen = write(ut->pipe[0], msg, len);
}

static void register_strophe() {
	uwsgi_register_alarm("strophe", alarm_strophe_init, alarm_strophe_func);
}

struct uwsgi_plugin strophe_plugin = {
	.name = "strophe",
	.on_load = register_strophe,
};
