/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
//
#include <math.h>
#include <time.h>
#include <net_helper.h>
#include <peerset.h>
#include <peersampler.h>
#include <peer.h>
#include <grapes_msg_types.h>
//
#include "compatibility/timer.h"
//
#include "topology.h"
#include "streaming.h"
#include "dbg.h"
#include "measures.h"
#include "xlweighter.h"
#include "streamer.h"
#include "node_addr.h"
#include "SDP_distribution.h"
#include "PeerSet/peerset_private.h"

static char source_session_id[SESSION_ID_SIZE];
static unsigned long source_session_version = 0;
static FILE *SDP_log;
static struct session_id_set *my_session_id_set;
static struct peerset *neighbourhood;


void SDP_spreading_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len, struct nodeID *myID){
    fprintf(stderr, "ENTRO IN SDP_spreading_message_parse\n");
    static struct SDP_spreading_message message;
    decode_SDP_spreading_message(buff, &message, len);
    add_session_id(message.session_id, my_session_id_set);
    struct session_id_set *remote_session_id_set = peerset_get_peer(neighbourhood,from)->session_id_set;
    fprintf(stderr, "SDP_spreading_message_parse: aggiungo il session_id al vicino\n");
    add_session_id(message.session_id, remote_session_id_set);
    continue_SDP_spread(buff, len, h, myID, message.session_id);
    fprintf(stderr, "FINE SDP_spreading_message_parse\n");
}
void SDP_signalling_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len,struct nodeID *myID){
    switch(buff[0]){
        case MSG_TYPE_SDP_SIGNALLING_OFFER:
            fprintf(stderr, "SDP_signalling_message_parse: RICEVUTO MESSAGGIO MSG_TYPE_SDP_SIGNALLING_OFFER\n"); 
            SDP_signalling_offer_message_parse(h, from, buff + 1, len - 1, myID);
            break;
        case MSG_TYPE_SDP_SIGNALLING_REQUEST:
            fprintf(stderr, "SDP_signalling_message_parse: RICEVUTO MESSAGGIO MSG_TYPE_SDP_SIGNALLING_REQUEST\n");     
            SDP_signalling_request_message_parse(h, from, buff + 1, len - 1, myID);
            break;
        default:
            fprintf(stderr, "SDP_signalling_message_parse: RICEVUTO MESSAGGIO MSG_TYPE_SDP_SIGNALLING SCONOSCIUTO\n");    
    }
}
void SDP_update_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len,struct nodeID *myID){
    static struct SDP_update_message message;
    decode_SDP_update_message(buff, &message, len);
}

void decode_SDP_spreading_message(const uint8_t *buff, struct SDP_spreading_message *message,int len){
    fprintf(stderr, "ENTRO IN decode_SDP_spreading_message\n");
    message->session_id = (char *)malloc(SESSION_ID_SIZE * sizeof(char));
    memcpy(message->session_id, buff, SESSION_ID_SIZE * sizeof(char));
    fprintf(stderr, "decode_SDP_spreading_message: session_id ricevuto:  %s\n", message->session_id);
    message->text = (char *)malloc((len - SESSION_ID_SIZE) * sizeof(char));
    memcpy(message->text, buff + SESSION_ID_SIZE * sizeof(char), (len - SESSION_ID_SIZE) * sizeof(char));
    fprintf(stderr, "decode_SDP_spreading_message: testo SDP ricevuto:  %s\n", message->text);
    file_SDP(message->session_id, message->text);
}
void decode_SDP_signalling_message(const uint8_t *buff, struct SDP_signalling_message *message,int len){
    fprintf(stderr, "decode_SDP_signalling_message: LUNGHEZZA DEL MESSAGGIO: %d\n", len);
    message->session_id_set = session_id_set_init();
    for(int i = 0; i < len; i+=SESSION_ID_SIZE){
        message->session_id_set->n_elements ++;
        memcpy(message->session_id_set->elements[i].session_id, buff + i, SESSION_ID_SIZE);
        fprintf(stderr, "decode_SDP_signalling_message: ID RECUPERATO: %s\n", buff + i);
    }
}
void decode_SDP_update_message(const uint8_t *buff, struct SDP_update_message *message,int len){
     uint8_t num_sessions;
    int *dim_array;
    char **session_id_array;
    message->session_id_set = session_id_set_init();
    uint8_t **SDP_array;
    num_sessions = buff[0];
    session_id_array = (char **)malloc(num_sessions * sizeof(char*));
    dim_array = (int *)malloc(num_sessions * sizeof(int));
    memcpy(dim_array, buff + 1 + num_sessions * SESSION_ID_SIZE * sizeof(char), num_sessions * sizeof(int));
    for(int i = 0; i < num_sessions; i++){
        //DA METTERE A POSTO QUI
        /*message->session_id_set->elements[i] = (char*)malloc(SESSION_ID_SIZE * sizeof(char));
        memcpy(message->session_id_set->elements[i], buff + 1 + i*SESSION_ID_SIZE*sizeof(char), SESSION_ID_SIZE);*/
        message->session_id_set->n_elements ++;
    }
    session_id_array = buff + 1;
    fprintf(stderr, "ncast_parse_SDP: NUMERO FLUSSI RICEVUTI: %d\n", num_sessions);
    for(int i = 0; i < num_sessions; i++){
        fprintf(stderr, "ncast_parse_SDP: DIMENSIONE DEL SDP RICEVUTO: %d\n", dim_array[i]);
    }
    for(int i = 0; i < num_sessions; i++){
        fprintf(stderr, "ncast_parse_SDP: ID DEL SDP RICEVUTO: %s\n", &session_id_array[i]);
    }
    for(int i = 0; i < num_sessions; i++){
        char *str;
        if(i == 0){
            str = (char *)malloc(dim_array[i] * sizeof(char));
            memcpy(str, buff + 2 + num_sessions * SESSION_ID_SIZE * sizeof(char) + num_sessions * sizeof(int), dim_array[i] * sizeof(char));
            str[dim_array[i]] = '\0';
            fprintf(stderr, "ncast_parse_SDP: SDP RICEVUTO:\n%s\n", str);
        }else{
            str = (char *)malloc(dim_array[i] * sizeof(char));
            memcpy(str, buff + 2 + num_sessions * SESSION_ID_SIZE * sizeof(char) + num_sessions * sizeof(int) + dim_array[i - 1], dim_array[i] * sizeof(char));
            str[dim_array[i]] = '\0';
            fprintf(stderr, "ncast_parse_SDP: SDP RICEVUTO:\n%s\n", str);
        }
        char s[64];
        strcpy(s, "SDP");
        strcat(s + 3, &session_id_array[i]);
        FILE *file = fopen(s, "w");
        fputs(str, file);
    }

}

void SDP_source_init(const char *fname, struct nodeID *myID, struct peerset *neighbourhood_ref){
    fprintf(stderr, "ENTRO IN SDP_source_init\n");
    SDP_log_init();
    neighbourhood = neighbourhood_ref;
    source_session_version = get_timestamp();
    prepare_session_id(fname, myID);
    prepare_SDP(fname, myID);
    init_my_session_id_set();
    add_session_id_source(source_session_id, my_session_id_set);
}

void SDP_init(struct peerset *neighbourhood_ref){
    fprintf(stderr, "ENTRO IN SDP_init\n");
    SDP_log_init();
    init_my_session_id_set();
    neighbourhood = neighbourhood_ref;
}

void prepare_SDP(const char *fname, struct nodeID *myID)
{
    fprintf(stderr, "prepare_SDP: PREPARAZIONE DEL FILE SDP...\n");
    char s[64];
    strcpy(s, "SDP");
    char ip[16];
    node_ip(myID, ip, SESSION_ID_SIZE);
    fprintf(stderr, "prepare_SDP: MIO IP: %s\n", ip);
    strcat(s + 3, source_session_id);
    FILE *f = fopen(s, "w");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }
    fprintf(f, "v=0\n");
    fprintf(f, "o= - %lu %lu IN IP4 %s\n", source_session_version, source_session_version, ip);
    fprintf(f, "ESP H264+AAC STREAM\n");
    fprintf(f, "c=IN IP4 %s\n", ip);
    fprintf(f, "t=0 0\n");
    fprintf(f, "m=video 7000 RTP/AVP 96\n");
    fprintf(f, "a=rtpmap:96 H264/90000\n");
    fprintf(f, "a=fmtp:96 media=video; clock-rate=90000; encoding-name=H264; sprop-parameter-sets=Z2QAH6zZQFAFuwEQAAADABAAAAMDAPGDGWA=,aOvjyyLA\n");
    fprintf(f, "a=control:trackID=1\n");
    fprintf(f, "m=audio 7002 RTP/AVP 96\n");
    fprintf(f, "a=rtpmap:96 MP4A-LATM/48000\n");
    fprintf(f, "a=fmtp:96 media=audio; clock-rate=48000; encoding-name=MP4A-LATM; cpresent=0; config=40002320adca00; payload=96\n");
    fprintf(f, "a=control:trackID=2\0");
    fclose(f);
    fprintf(stderr, "prepare_SDP: FILE GENERATO: %s\n", s);
}

void prepare_session_id(const char *fname, struct nodeID *myID){
    char ip[16];
    for(int i = 0; i < SESSION_ID_SIZE; i++){
        source_session_id[i] = '-';
    }
    fprintf(stderr, "prepare_session_id: SESSION_ID_GENERATO: %s\n", source_session_id);
    node_ip(myID, ip, SESSION_ID_SIZE);
    strcpy(source_session_id + 16, ip);
    srand(time(NULL) + getpid());
    for(int i = 0; i < 16 - 1; i++){
        source_session_id[i] = 'A' + (rand() % 26);
    }
    source_session_id[SESSION_ID_SIZE - 1] = '\0';
    fprintf(stderr, "prepare_session_id: SESSION_ID_GENERATO: %s\n", source_session_id);
}

unsigned long get_timestamp()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
    return time_in_micros;
}

void continue_SDP_spread(const uint8_t *buff, int len, struct peerset *neighbourhood, struct nodeID *myID, char *session_id){
    fprintf(stderr, "ENTRO IN continue_SDP_spread\n");
    uint8_t *pkt;
    pkt = (uint8_t*)malloc(len * sizeof(uint8_t) + 2 * sizeof(char));
    pkt[0] = MSG_TYPE_SDP;
    pkt[1] = MSG_TYPE_SDP_SPREADING;
    memcpy(pkt + 2, buff, len * sizeof(char));
    struct peer **peerset;
    char address[64];
    peerset = peerset_get_peers(neighbourhood);
    for(int i = 0; i < neighbourhood->n_elements; i++){
        struct nodeID *dst_ID = peerset[i]->id;
        node_addr(dst_ID, address, SESSION_ID_SIZE);
        struct session_id_set *remote_session_id_set;
        remote_session_id_set = peerset_get_peer(neighbourhood,dst_ID)->session_id_set;
        fprintf(stderr, "continue_SDP_spread: aggiungo il session_id al vicino\n");
        if(add_session_id(session_id, remote_session_id_set)){
            send_to_peer(myID, dst_ID, pkt, 2 * sizeof(uint8_t) + SESSION_ID_SIZE * sizeof(char) + len * sizeof(uint8_t) + 2 * sizeof(char));
            fprintf(stderr, "continue_SDP_spread: inviato messaggio MSG_TYPE_SDP_SPREADING (SDP %s) al vicino: %s\nil testo è:\n%s\n", (pkt + 2), address, (pkt + 2 + SESSION_ID_SIZE));
        }else{
            fprintf(stderr, "continue_SDP_spread: IL VICINO HA GIA QUEL SESSION_ID; MESSAGGIO NON INVIATO\n");
        }
        fprintf(stderr, "continue_SDP_spread: IL VICINO HA %d ELEMENTI\n", peerset_get_peer(neighbourhood,dst_ID)->session_id_set->n_elements);
        fprintf(stderr, "continue_SDP_spread: il vicino ora ha %d elementi\n", remote_session_id_set->n_elements);
    }
}

void SDP_spread(const struct peerset *neighbourhood, struct nodeID *myID){
    fprintf(stderr, "ENTRO IN SDP_spread\n");
    uint8_t *pkt;
    char s[64];
    strcpy(s, "SDP");
    strcat(s, source_session_id);
    FILE *f = fopen(s, "r");
    fprintf(stderr, "SDP_spread: FILE APERTO: %s\n", s);
    fseek(f, 0, SEEK_END);
    int payload_size = (int)ftell(f);
    fprintf(stderr, "SDP_spread: DIMENSIONE FILE: %d\n", payload_size);
    char *buffer = (char *)malloc(payload_size * sizeof(char));
    rewind(f);
    fread(buffer, payload_size, 1 , f);
    buffer[payload_size] = '\0';
    ++payload_size;
    fclose(f);
    pkt = (uint8_t*)malloc(2 * sizeof(uint8_t) + SESSION_ID_SIZE * sizeof(char)  + payload_size * sizeof(char));
    pkt[0] = MSG_TYPE_SDP;
    pkt[1] = MSG_TYPE_SDP_SPREADING;
    memcpy(pkt + 2, source_session_id, SESSION_ID_SIZE * sizeof(char));
    memcpy(pkt + 2 + SESSION_ID_SIZE, buffer, payload_size);
    struct peer **peerset;
    char address[64];
    peerset = peerset_get_peers(neighbourhood);
    for(int i = 0; i < neighbourhood->n_elements; i++){
        struct nodeID *dst_ID = peerset[i]->id;
        node_addr(dst_ID, address, SESSION_ID_SIZE);
        send_to_peer(myID, dst_ID, pkt, 2 * sizeof(uint8_t) + SESSION_ID_SIZE * sizeof(char) + + payload_size * sizeof(char));
        fprintf(stderr, "SDP_spread: inviato messaggio MSG_TYPE_SDP_SPREADING (SDP %s) al vicino: %s\nil testo è:\n%s\n", (pkt + 2), address, (pkt + 2 + SESSION_ID_SIZE));
        struct session_id_set *remote_session_id_set;
        remote_session_id_set = peerset_get_peer(neighbourhood,dst_ID)->session_id_set;
        fprintf(stderr, "SDP_spread: IL VICINO HA %d ELEMENTI\n", peerset_get_peer(neighbourhood,dst_ID)->session_id_set->n_elements);
        fprintf(stderr, "SDP_spread: aggiungo il session_id al vicino\n");
        add_session_id(source_session_id, remote_session_id_set);
        fprintf(stderr, "SDP_spread: il vicino ora ha %d elementi\n", remote_session_id_set->n_elements);
    }
    fprintf(stderr, "FINE SDP_spread\n");
}

void SDP_log_init(){
    /*fclose(fopen("SDP_log", "w"));
    SDP_log = fopen("SDP_log", "a");*/
}

int add_session_id(char *session_id, struct session_id_set *session_id_set){
    fprintf(stderr, "ENTRO IN add_session_id\n");
    /*if(session_id_set == NULL){
        fprintf(stderr, "add_session_id: FACCIO LA INIT\n");
        init_session_id_set(session_id_set);
        fprintf(stderr, "init_session_id_set: ora ci sono %d elementi\n", session_id_set->n_elements);
        fprintf(stderr, "add_session_id: INIT CONCLUSA\n");
    }*/
    fprintf(stderr, "add_session_id: N ELEMENTI: %d\n", session_id_set->n_elements);
    for(int i = 0; i < session_id_set->n_elements; i++){
        fprintf(stderr, "add_session_id: DENTRO CICLO: ELEMENTI DA COMPARARE: %s e %s\n", session_id, session_id_set->elements[i].session_id);
        if(strcmp(session_id, session_id_set->elements[i].session_id) == 0){
            session_id_set->elements[i].last_time_recieved = get_timestamp();
            return 0;
        }
    }
    fprintf(stderr, "add_session_id: FINE CICLO\n");
    memcpy(session_id_set->elements[session_id_set->n_elements].session_id, session_id, SESSION_ID_SIZE);
    session_id_set->elements[session_id_set->n_elements].last_time_recieved = get_timestamp();
    session_id_set->elements[session_id_set->n_elements].am_I_source = 0;
    session_id_set->n_elements = session_id_set->n_elements + 1;
    fprintf(stderr, "add_session_id: AGGIUNTO L'ID %s\n", session_id);
    fprintf(stderr, "add_session_id: ora ci sono %d elementi\n", session_id_set->n_elements);
    return 1;
}

void init_my_session_id_set(){
    my_session_id_set = (struct session_id_set *)malloc(sizeof(struct session_id_set));
    my_session_id_set->n_elements = 0;
}

void init_session_id_set(struct session_id_set *session_id_set){
    session_id_set = (struct session_id_set *)malloc(sizeof(struct session_id_set));
    session_id_set->n_elements = 0;
    fprintf(stderr, "init_session_id_set: ora ci sono %d elementi\n", session_id_set->n_elements);
}

void add_session_id_source(char *session_id, struct session_id_set *session_id_set){
    memcpy(session_id_set->elements[session_id_set->n_elements].session_id, session_id, SESSION_ID_SIZE);
    session_id_set->elements[session_id_set->n_elements].last_time_recieved = get_timestamp();
    session_id_set->elements[session_id_set->n_elements].am_I_source = 1;
    session_id_set->n_elements = session_id_set->n_elements + 1;
    fprintf(stderr, "add_session_id: ora ci sono %d elementi\n", session_id_set->n_elements);
}

struct session_id_set *session_id_set_init()
{
  
  struct session_id_set *session_id_set;
  session_id_set = malloc(sizeof(struct session_id_set));
  session_id_set->n_elements = 0;

  return session_id_set;
}

void file_SDP(char *session_id, char *text){
    fprintf(stderr, "ENTRO IN file_SDP\n");
    char s[64];
    strcpy(s, "SDP");
    strcat(s + 3, session_id);
    FILE *f = fopen(s, "w");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }
    fprintf(f, "%s", text);
    fclose(f);
    fprintf(stderr, "file_SDP: FILE GENERATO: %s; session_id: %s\n", s, session_id);
}

void SDP_existence_spread(const struct peerset *neighbourhood, struct nodeID *myID){
    fprintf(stderr, "ENTRO IN SDP_existence_spread\n");
    uint8_t *pkt;
    char **local_id_set = (char **)malloc(SESSION_ID_SET_MAX_SIZE * sizeof(char*));
    for(int i = 0; i < my_session_id_set->n_elements; i++){
        local_id_set[i] = (char *)malloc(SESSION_ID_SIZE * sizeof(char));
        memcpy(local_id_set[i], my_session_id_set->elements[i].session_id, SESSION_ID_SIZE);
        fprintf(stderr, "SDP_existence_spread: AGGIUNTO AL PACCHETTO L'ID %s\n", my_session_id_set->elements[i].session_id);
    }
    pkt = (uint8_t*)malloc(3 * sizeof(char) + my_session_id_set->n_elements * SESSION_ID_SIZE * sizeof(char));
    pkt[0] = MSG_TYPE_SDP;
    pkt[1] = MSG_TYPE_SDP_SIGNALLING;
    pkt[2] = MSG_TYPE_SDP_SIGNALLING_OFFER;
    int len = 3;
    for(int i = 0; i < my_session_id_set->n_elements; i++){
        memcpy(pkt + 3 + i*my_session_id_set->n_elements*SESSION_ID_SIZE*sizeof(char), local_id_set[i], SESSION_ID_SIZE*sizeof(char));
        len = len + SESSION_ID_SIZE*sizeof(char);
    }
    fprintf(stderr, "SDP_existence_spread: PACCHETTO PREPARATO\n");
    struct peer **peerset;
    char address[64];
    peerset = peerset_get_peers(neighbourhood);
    for(int i = 0; i < neighbourhood->n_elements; i++){
        struct nodeID *dst_ID = peerset[i]->id;
        node_addr(dst_ID, address, SESSION_ID_SIZE);
        send_to_peer(myID, dst_ID, pkt, len);
        fprintf(stderr, "SDP_existence_spread: inviato messaggio MSG_TYPE_SDP_SIGNALLING_OFFER al vicino: %s\n", address);
        struct session_id_set *remote_session_id_set;
        remote_session_id_set = peerset_get_peer(neighbourhood,dst_ID)->session_id_set;
        for(int i = 0; i < my_session_id_set->n_elements; i++){
            fprintf(stderr, "SDP_existence_spread: IL VICINO HA %d ELEMENTI\n", peerset_get_peer(neighbourhood,dst_ID)->session_id_set->n_elements);
            fprintf(stderr, "SDP_existence_spread: aggiungo il session_id al vicino\n");
            add_session_id(my_session_id_set->elements[i].session_id, remote_session_id_set);
            fprintf(stderr, "SDP_existence_spread: il vicino ora ha %d elementi\n", remote_session_id_set->n_elements);
        }
    }
}

void SDP_signalling_request_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len,struct nodeID *myID){
    static struct SDP_signalling_message message;
    decode_SDP_signalling_message(buff, &message, len);
    if(message.session_id_set->n_elements > 0){
        fprintf(stderr, "SDP_signalling_request_message_parse: RICEVUTO DAL VICINO RICHIESTA CON %d SESSION_ID\n", message.session_id_set->n_elements); 
        send_SDP_update(from, myID, &message);
    }else{
        fprintf(stderr, "SDP_signalling_request_message_parse: LA RICHIESTA CONTIENE 0 ELEMENTI\n");
    }
}
void SDP_signalling_offer_message_parse(struct peerset *h,struct nodeID *from,const uint8_t *buff,int len,struct nodeID *myID){
    static struct SDP_signalling_message message;
    struct session_id_set *request_session_id_set;
    request_session_id_set = session_id_set_init();
    decode_SDP_signalling_message(buff, &message, len);
    struct session_id_set *remote_session_id_set;
    remote_session_id_set = peerset_get_peer(neighbourhood,from)->session_id_set;
    for(int i = 0; i < message.session_id_set->n_elements; i++){
        if(add_session_id(message.session_id_set->elements[i].session_id, my_session_id_set)){
            add_session_id(message.session_id_set->elements[i].session_id, request_session_id_set);
        }
        for(int i = 0; i < message.session_id_set->n_elements; i++){
            fprintf(stderr, "SDP_signalling_offer_message_parse: IL VICINO HA %d ELEMENTI\n", peerset_get_peer(neighbourhood,from)->session_id_set->n_elements);
            fprintf(stderr, "SDP_signalling_offer_message_parse: aggiungo il session_id al vicino\n");
            add_session_id(message.session_id_set->elements[i].session_id, remote_session_id_set);
            fprintf(stderr, "SDP_signalling_offer_message_parse: il vicino ora ha %d elementi\n", remote_session_id_set->n_elements);
        }
    }
    SDP_signalling_create_request(from, myID, request_session_id_set);
}

void SDP_signalling_create_request(struct nodeID *from, struct nodeID *myID, struct session_id_set *session_id_set){
    fprintf(stderr, "ENTRO IN SDP_signalling_create_request\n");
    uint8_t *pkt;
    char **id_set = (char **)malloc(SESSION_ID_SET_MAX_SIZE * sizeof(char*));
    for(int i = 0; i < session_id_set->n_elements; i++){
        id_set[i] = (char *)malloc(SESSION_ID_SIZE * sizeof(char));
        memcpy(id_set[i], session_id_set->elements[i].session_id, SESSION_ID_SIZE);
        fprintf(stderr, "SDP_signalling_create_request: AGGIUNTO AL PACCHETTO L'ID %s\n", session_id_set->elements[i].session_id);
    }
    pkt = (uint8_t*)malloc(3 * sizeof(char) + session_id_set->n_elements * SESSION_ID_SIZE * sizeof(char));
    pkt[0] = MSG_TYPE_SDP;
    pkt[1] = MSG_TYPE_SDP_SIGNALLING;
    pkt[2] = MSG_TYPE_SDP_SIGNALLING_REQUEST;
    int len = 3;
    for(int i = 0; i < session_id_set->n_elements; i++){
        memcpy(pkt + 3 + i*session_id_set->n_elements*SESSION_ID_SIZE*sizeof(char), id_set[i], SESSION_ID_SIZE*sizeof(char));
        len = len + SESSION_ID_SIZE*sizeof(char);
    }
    fprintf(stderr, "SDP_signalling_create_request: PACCHETTO PREPARATO\n");
    struct peer **peerset;
    char address[64];
    peerset = peerset_get_peers(neighbourhood);
    for(int i = 0; i < neighbourhood->n_elements; i++){
        struct nodeID *dst_ID = peerset[i]->id;
        node_addr(dst_ID, address, SESSION_ID_SIZE);
        send_to_peer(myID, dst_ID, pkt, len);
        fprintf(stderr, "SDP_signalling_create_request: inviato messaggio MSG_TYPE_SDP_SIGNALLING_REQUEST al vicino: %s\n", address);
        fprintf(stderr, "FINE SDP_signalling_create_request\n");
    }
}

void send_SDP_update(struct nodeID *from, struct nodeID *myID, struct SDP_signalling_message *message){
    fprintf(stderr, "ENTRO IN send_SDP_update\n");
    uint8_t *pkt;
    int num_sessions = message->session_id_set->n_elements;
    int *dim_array = (int *)malloc(num_sessions * sizeof(int));
    char ** requested_SDP = (char **)malloc(num_sessions * sizeof(char*));
    FILE **f = (FILE **)malloc(num_sessions * sizeof(FILE*));
    int payload_size = 0;
    for(int i = 0; i < num_sessions; i++){
        fprintf(stderr, "RECUPERO FILE PER L'ID%d\n", message->session_id_set->elements[i].session_id);
        char s[64];
        strcpy(s, "SDP");
        strcat(s, message->session_id_set->elements[i].session_id);
        f[i] = fopen(s, "r");
        fseek(f[i], 0, SEEK_END);
        int lengthOfFile = (int)ftell(f[i]);
        payload_size += lengthOfFile;
        dim_array[i] = lengthOfFile;
        requested_SDP[i] = (char *)malloc(SESSION_ID_SIZE * sizeof(char));
        requested_SDP[i] = message->session_id_set->elements[i].session_id;
        fprintf(stderr, "MESSO NEL VETTORE: %s\n", requested_SDP[i]);
        fprintf(stderr, "send_SDP_update: DIMENSIONE DEL RELATIVO FILE SDP: %d\n", lengthOfFile);
        rewind(f[i]);
    }
    char *buffer = (char *)malloc(payload_size * sizeof(char));
    for(int i = 0; i < num_sessions; i++){
        if(i == 0)
            fread(buffer, dim_array[i], 1 , f[i]);
        else
            fread(buffer + dim_array[i - 1], dim_array[i], 1 , f[i]);
        fclose(f[i]);
    }
    pkt = (uint8_t*)malloc(2*sizeof(uint8_t) + sizeof(int) + num_sessions * SESSION_ID_SIZE * sizeof(char) + num_sessions * sizeof(int) + payload_size * sizeof(char));
    pkt[0] = MSG_TYPE_SDP;
    pkt[1] = MSG_TYPE_SDP_UPDATE;
    pkt[2] = num_sessions;  
    for(int i = 0; i < num_sessions; i++){
        memcpy(pkt + 3 + i * SESSION_ID_SIZE * sizeof(char), requested_SDP[i], SESSION_ID_SIZE * sizeof(char));
    }
    memcpy(pkt + 3 + SESSION_ID_SIZE * sizeof(char) * num_sessions, dim_array, num_sessions * sizeof(int));
    memcpy(pkt + 3 + SESSION_ID_SIZE * sizeof(char) * num_sessions + num_sessions * sizeof(int), buffer, payload_size);
    send_to_peer(myID, from, pkt, 3 * sizeof(uint8_t) + num_sessions * SESSION_ID_SIZE * sizeof(char) + num_sessions * sizeof(int) + payload_size * sizeof(char));
    fprintf(stderr, "send_SDP_update: SPEDITO MESSAGGIO DI TIPO MSG_TYPE_SDP_UPDATE\n");
}