/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SDP_distribution.h
 * Author: stefano
 *
 * Created on 22 luglio 2017, 9.45
 */

#ifndef SDP_DISTRIBUTION_H
#define SDP_DISTRIBUTION_H

#define MSG_TYPE_SDP_SPREADING   0x23
#define MSG_TYPE_SDP_SIGNALLING   0x24
#define MSG_TYPE_SDP_UPDATE   0x25
#define MSG_TYPE_SDP_SIGNALLING_OFFER   0x26
#define MSG_TYPE_SDP_SIGNALLING_REQUEST   0x27
#define SESSION_ID_SIZE   32
#define TIME_TO_SPREAD_SDP 100
#define TIME_TO_REMOVE_SDP 100000000
#define SESSION_ID_SET_MAX_SIZE 100
#define KEEP_ALIVE_PERIOD 2000

typedef struct SDP_spreading_message {
    char *session_id;
    char *text;
};

typedef struct SDP_signalling_message {
    struct session_id_set *session_id_set;
};

typedef struct SDP_update_message {
    struct session_id_set *session_id_set;
    char * SDP[SESSION_ID_SET_MAX_SIZE];
};

typedef struct session_id_entry {
    char session_id[SESSION_ID_SIZE];
    unsigned long last_time_recieved;
    uint8_t am_I_source;
};

typedef struct session_id_set {
    struct session_id_entry elements[SESSION_ID_SET_MAX_SIZE];
    int n_elements;
};


/*typedef struct session_node_id_set {
    struct nodeID *node_ID;
    struct session_id_set *session_id_set;
};

typedef struct session_id_set_map {
    struct session_node_id_set *session_node_id_set;
    int n_elements;
};*/

void SDP_spreading_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len,struct nodeID *myID);
void SDP_signalling_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len,struct nodeID *myID);
void SDP_update_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len,struct nodeID *myID);
void decode_SDP_spreading_message(const uint8_t *buff, struct SDP_spreading_message *message,int len);
void decode_SDP_signalling_message(const uint8_t *buff, struct SDP_signalling_message *message,int len);
void decode_SDP_update_message(const uint8_t *buff, struct SDP_update_message *message,int len);
void SDP_source_init(const char *fname, struct nodeID *myID, struct peerset *neighbourhood_ref);
void prepare_SDP(const char *fname, struct nodeID *myID);
void prepare_session_id(const char *fname, struct nodeID *myID);
unsigned long get_timestamp();
void SDP_spread(const struct peerset *neighbourhood, struct nodeID *myID);
void SDP_log_init();
void SDP_init(struct peerset *neighbourhood_ref);
int add_session_id(char *session_id, struct session_id_set *session_id_set);
void init_my_session_id_set();
void add_session_id_source(char *session_id, struct session_id_set *session_id_set);
void init_session_id_set(struct session_id_set *session_id_set);
struct session_id_set *session_id_set_init();
void file_SDP(char *session_id, char *text);
void SDP_existence_spread(const struct peerset *neighbourhood, struct nodeID *myID);
void SDP_signalling_request_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len,struct nodeID *myID);
void SDP_signalling_offer_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len,struct nodeID *myID);
void SDP_signalling_create_request(struct nodeID *from, struct nodeID *myID, struct session_id_set *session_id_set);
void send_SDP_update(struct nodeID *from, struct nodeID *myID, struct SDP_signalling_message *message);

#endif /* SDP_DISTRIBUTION_H */
