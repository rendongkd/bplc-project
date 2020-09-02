/* Process model C form file: bplc_mac.pr.c */
/* Portions of this file copyright 1986-2008 by OPNET Technologies, Inc. */



/* This variable carries the header into the object file */
const char bplc_mac_pr_c [] = "MIL_3_Tfile_Hdr_ 145A 30A op_runsim 7 5EFD943E 5EFD943E 1 PC-201907112246 Administrator 0 0 none none 0 0 none 0 0 0 0 0 0 0 0 1bcc 1                                                                                                                                                                                                                                                                                                                                                                                          ";
#include <string.h>



/* OPNET system definitions */
#include <opnet.h>



/* Header Block */

#include "string.h"

#define	instrm_from_phy	    0
#define	instrm_from_intf	1
#define	instrm_to_phy	    0
#define	instrm_to_intf		1

#define beacon_code			1000
#define csma_code			2000
#define tdma_code			3000
#define bind_csma_code		4000
#define data_code		    5000
#define quit_csma_code		6000
#define quit_bind_csma_code		7000
#define pco_send_beacon_code 8000
#define level2_send_beacon_code 9000
#define	 RX_INSTAT 			0


#define slot_time           1E-3;//10MS
#define LOW_ARVL 			(op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == instrm_from_phy)
#define UP_ARVL 			(op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == instrm_from_intf)

#define SEND_BEACON         (op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == beacon_code)
#define ENTER_CSMA          (op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == csma_code)
#define ENTER_TDMA          (op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == tdma_code)
#define ENTER_BIND_CSMA     (op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == bind_csma_code)
#define SEND_DATA         (op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == data_code)
#define QUIT_CSMA        (op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == quit_csma_code)
#define QUIT_BIND_CSMA        (op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == quit_bind_csma_code)
#define PCO_SEND            (op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == pco_send_beacon_code)
#define LEVEL2_SEND_BEACON      (op_intrpt_type() == OPC_INTRPT_SELF && op_intrpt_code() == level2_send_beacon_code)

#define	RX_BUSY				((op_intrpt_type()== OPC_INTRPT_STAT) && (op_intrpt_stat () == RX_INSTAT ) && op_stat_local_read (RX_INSTAT) == 1.0)
#define	RX_IDLE				((op_intrpt_type()== OPC_INTRPT_STAT) && (op_intrpt_stat () == RX_INSTAT ) && op_stat_local_read (RX_INSTAT) == 0.0)

double sta_data;//to record sta send data for sink
double cco_data;//to record cco send data for sink

enum NodeAttributes //node's attributes
{
	CCO, 
	PCO, 
	STA
};
typedef enum MAC_frame_type //MAC frame type
{
	CCO_beacon,
	PCO_beacon,
    STA_beacon
}MAC_frame_type;
typedef struct beacon_slot_table_element
{	
	int node_TEI;
	int beacon_num;
	int node_level;
}beacon_slot_table_element;
typedef struct TDMA_slot_table_element
{	
	int node_TEI;
	int TDMA_num;
}TDMA_slot_table_element;

Stathandle join_net_time_gshandle;

int  pco_node1[10]; //use by CCO
Packet*		pkptr1;

int  mac_send_nb = 0;
int  mac_forward_nb = 0;
int  mac_uplayer_nb = 0;

/* End of Header Block */

#if !defined (VOSD_NO_FIN)
#undef	BIN
#undef	BOUT
#define	BIN		FIN_LOCAL_FIELD(_op_last_line_passed) = __LINE__ - _op_block_origin;
#define	BOUT	BIN
#define	BINIT	FIN_LOCAL_FIELD(_op_last_line_passed) = 0; _op_block_origin = __LINE__;
#else
#define	BINIT
#endif /* #if !defined (VOSD_NO_FIN) */



/* State variable definitions */
typedef struct
	{
	/* Internal state tracking for FSM */
	FSM_SYS_STATE
	/* State Variables */
	int	                    		node_address                                    ;
	int	                    		dest_addr                                       ;
	Objid	                  		node_id                                         ;
	int	                    		flag                                            ;
	Stathandle	             		cco_send_data_packet_stathandle                 ;
	Stathandle	             		sta_send_data_packet_stathandle                 ;
	double	                 		cco_send_data                                   ;
	double	                 		sta_send_data                                   ;
	int	                    		rx_busy                                         ;
	int	                    		csma_key                                        ;
	int	                    		bind_csma_key                                   ;
	double	                 		cycle_time                                      ;
	int	                    		tei                                             ;
	Stathandle	             		sta_data_stathandle                             ;
	Stathandle	             		cco_data_stathandle                             ;
	int	                    		level_flag                                      ;
	int	                    		node_type                                       ;
	int	                    		next_hop[50]                                    ;
	int	                    		depth                                           ;
	int	                    		cco_rev_sta_data_flag                           ;
	int	                    		cco_to_sta_data_flag                            ;
	} bplc_mac_state;

#define node_address            		op_sv_ptr->node_address
#define dest_addr               		op_sv_ptr->dest_addr
#define node_id                 		op_sv_ptr->node_id
#define flag                    		op_sv_ptr->flag
#define cco_send_data_packet_stathandle		op_sv_ptr->cco_send_data_packet_stathandle
#define sta_send_data_packet_stathandle		op_sv_ptr->sta_send_data_packet_stathandle
#define cco_send_data           		op_sv_ptr->cco_send_data
#define sta_send_data           		op_sv_ptr->sta_send_data
#define rx_busy                 		op_sv_ptr->rx_busy
#define csma_key                		op_sv_ptr->csma_key
#define bind_csma_key           		op_sv_ptr->bind_csma_key
#define cycle_time              		op_sv_ptr->cycle_time
#define tei                     		op_sv_ptr->tei
#define sta_data_stathandle     		op_sv_ptr->sta_data_stathandle
#define cco_data_stathandle     		op_sv_ptr->cco_data_stathandle
#define level_flag              		op_sv_ptr->level_flag
#define node_type               		op_sv_ptr->node_type
#define next_hop                		op_sv_ptr->next_hop
#define depth                   		op_sv_ptr->depth
#define cco_rev_sta_data_flag   		op_sv_ptr->cco_rev_sta_data_flag
#define cco_to_sta_data_flag    		op_sv_ptr->cco_to_sta_data_flag

/* These macro definitions will define a local variable called	*/
/* "op_sv_ptr" in each function containing a FIN statement.	*/
/* This variable points to the state variable data structure,	*/
/* and can be used from a C debugger to display their values.	*/
#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE
#define FIN_PREAMBLE_DEC	bplc_mac_state *op_sv_ptr;
#define FIN_PREAMBLE_CODE	\
		op_sv_ptr = ((bplc_mac_state *)(OP_SIM_CONTEXT_PTR->_op_mod_state_ptr));


/* No Function Block */

#if !defined (VOSD_NO_FIN)
enum { _op_block_origin = __LINE__ };
#endif

/* Undefine optional tracing in FIN/FOUT/FRET */
/* The FSM has its own tracing code and the other */
/* functions should not have any tracing.		  */
#undef FIN_TRACING
#define FIN_TRACING

#undef FOUTRET_TRACING
#define FOUTRET_TRACING

#if defined (__cplusplus)
extern "C" {
#endif
	void bplc_mac (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Obtype _op_bplc_mac_init (int * init_block_ptr);
	void _op_bplc_mac_diag (OP_SIM_CONTEXT_ARG_OPT);
	void _op_bplc_mac_terminate (OP_SIM_CONTEXT_ARG_OPT);
	VosT_Address _op_bplc_mac_alloc (VosT_Obtype, int);
	void _op_bplc_mac_svar (void *, const char *, void **);


#if defined (__cplusplus)
} /* end of 'extern "C"' */
#endif




/* Process model interrupt handling procedure */


void
bplc_mac (OP_SIM_CONTEXT_ARG_OPT)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	FIN_MT (bplc_mac ());

		{
		/* Temporary Variables */
		Packet*		pkptr;
		Packet*		pktptr;
		Packet*     pk;
		
		int         type;
		int			src;
		int			dest;
		
		double beacon_slot_time=5;
		double csma_slot_time=15;
		double tdma_slot_time=17;
		double bind_csma_slot_time=20;
		double distance=0.0;
		
		int    node_type2 = 0;
		int    i = 0, j = 0;
		
		char   pco_node[6];
		
		
		/* End of Temporary Variables */


		FSM_ENTER ("bplc_mac")

		FSM_BLOCK_SWITCH
			{
			/*---------------------------------------------------------*/
			/** state (init) enter executives **/
			FSM_STATE_ENTER_FORCED_NOLABEL (0, "init", "bplc_mac [init enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [init enter execs]", state0_enter_exec)
				{
				node_id = op_topo_parent(op_id_self());
				op_ima_obj_attr_get(node_id,"destination_address",&dest_addr);//to get dest from attribute
				op_ima_obj_attr_get(node_id,"node_address",&node_address);
				
				node_type = 2;  //init, STA
				op_ima_obj_attr_get(node_id,"Node Type", &node_type);
				
				flag=0;//入网标志，0代表没有入网
				cco_rev_sta_data_flag=0;//up line data rcv not successful
				cco_to_sta_data_flag=0;//sta do not rcv cc0's data
				
				level_flag=0;//梯度标志，CCO标志为0
				cco_send_data_packet_stathandle = op_stat_reg ("CCO Sent (packets)",OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
				sta_send_data_packet_stathandle	= op_stat_reg ("STA Sent (packets)",OPC_STAT_INDEX_NONE, OPC_STAT_LOCAL);
				
				join_net_time_gshandle = op_stat_reg ("Join Net Time", OPC_STAT_INDEX_NONE, OPC_STAT_GLOBAL);
				
				sta_send_data = 0;
				cco_send_data = 0;
				
				if(node_address == 0)
					{
					op_intrpt_schedule_self(op_sim_time() + 1, beacon_code);//CCO上电后，进行1S的网间协调监听，以便发现是否存在邻居网络
					
					}
				else;
				
				csma_key=1;//close csma period
				bind_csma_key=0;//open
				cycle_time=20;
				tei=0;
				sta_data=0;
				cco_data=0;
				depth = 1000;
				next_hop[0] = -2;
				
				if(node_address == 0)  //CCO, init
				{
					for(i=0; i<10; i++)
						pco_node1[i]=-1;
				}
				
				for(i=0; i<50; i++)
					next_hop[i] = -2;
				
				}
				FSM_PROFILE_SECTION_OUT (state0_enter_exec)

			/** state (init) exit executives **/
			FSM_STATE_EXIT_FORCED (0, "init", "bplc_mac [init exit execs]")


			/** state (init) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "init", "idle", "tr_7", "bplc_mac [init -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (idle) enter executives **/
			FSM_STATE_ENTER_UNFORCED (1, "idle", state1_enter_exec, "bplc_mac [idle enter execs]")

			/** blocking after enter executives of unforced state. **/
			FSM_EXIT (3,"bplc_mac")


			/** state (idle) exit executives **/
			FSM_STATE_EXIT_UNFORCED (1, "idle", "bplc_mac [idle exit execs]")


			/** state (idle) transition processing **/
			FSM_PROFILE_SECTION_IN ("bplc_mac [idle trans conditions]", state1_trans_conds)
			FSM_INIT_COND (UP_ARVL)
			FSM_TEST_COND (LOW_ARVL)
			FSM_TEST_COND (SEND_BEACON)
			FSM_TEST_COND (ENTER_CSMA)
			FSM_TEST_COND (ENTER_TDMA)
			FSM_TEST_COND (ENTER_BIND_CSMA)
			FSM_TEST_COND (RX_IDLE)
			FSM_TEST_COND (RX_BUSY)
			FSM_TEST_COND (SEND_DATA)
			FSM_TEST_COND (QUIT_BIND_CSMA)
			FSM_TEST_COND (PCO_SEND)
			FSM_TEST_COND (LEVEL2_SEND_BEACON)
			FSM_DFLT_COND
			FSM_TEST_LOGIC ("idle")
			FSM_PROFILE_SECTION_OUT (state1_trans_conds)

			FSM_TRANSIT_SWITCH
				{
				FSM_CASE_TRANSIT (0, 2, state2_enter_exec, ;, "UP_ARVL", "", "idle", "up_arvl", "tr_9", "bplc_mac [idle -> up_arvl : UP_ARVL / ]")
				FSM_CASE_TRANSIT (1, 3, state3_enter_exec, ;, "LOW_ARVL", "", "idle", "low_arvl", "tr_10", "bplc_mac [idle -> low_arvl : LOW_ARVL / ]")
				FSM_CASE_TRANSIT (2, 4, state4_enter_exec, ;, "SEND_BEACON", "", "idle", "cco_send_beacon", "tr_13", "bplc_mac [idle -> cco_send_beacon : SEND_BEACON / ]")
				FSM_CASE_TRANSIT (3, 5, state5_enter_exec, ;, "ENTER_CSMA", "", "idle", "csma", "enter_csma", "bplc_mac [idle -> csma : ENTER_CSMA / ]")
				FSM_CASE_TRANSIT (4, 6, state6_enter_exec, ;, "ENTER_TDMA", "", "idle", "tdma", "tr_17", "bplc_mac [idle -> tdma : ENTER_TDMA / ]")
				FSM_CASE_TRANSIT (5, 7, state7_enter_exec, ;, "ENTER_BIND_CSMA", "", "idle", "bind_csma", "tr_21", "bplc_mac [idle -> bind_csma : ENTER_BIND_CSMA / ]")
				FSM_CASE_TRANSIT (6, 9, state9_enter_exec, ;, "RX_IDLE", "", "idle", "rx_idle", "tr_23", "bplc_mac [idle -> rx_idle : RX_IDLE / ]")
				FSM_CASE_TRANSIT (7, 8, state8_enter_exec, ;, "RX_BUSY", "", "idle", "rx_busy", "tr_25", "bplc_mac [idle -> rx_busy : RX_BUSY / ]")
				FSM_CASE_TRANSIT (8, 10, state10_enter_exec, ;, "SEND_DATA", "", "idle", "send_data", "tr_27", "bplc_mac [idle -> send_data : SEND_DATA / ]")
				FSM_CASE_TRANSIT (9, 11, state11_enter_exec, ;, "QUIT_BIND_CSMA", "", "idle", "quit_bind_csma", "tr_31", "bplc_mac [idle -> quit_bind_csma : QUIT_BIND_CSMA / ]")
				FSM_CASE_TRANSIT (10, 12, state12_enter_exec, ;, "PCO_SEND", "", "idle", "pco_send_beacon", "tr_34", "bplc_mac [idle -> pco_send_beacon : PCO_SEND / ]")
				FSM_CASE_TRANSIT (11, 13, state13_enter_exec, ;, "LEVEL2_SEND_BEACON", "", "idle", "level2_send_beacon", "tr_36", "bplc_mac [idle -> level2_send_beacon : LEVEL2_SEND_BEACON / ]")
				FSM_CASE_TRANSIT (12, 1, state1_enter_exec, ;, "default", "", "idle", "idle", "tr_12", "bplc_mac [idle -> idle : default / ]")
				}
				/*---------------------------------------------------------*/



			/** state (up_arvl) enter executives **/
			FSM_STATE_ENTER_FORCED (2, "up_arvl", state2_enter_exec, "bplc_mac [up_arvl enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [up_arvl enter execs]", state2_enter_exec)
				{
				pkptr=op_pk_get(instrm_from_intf);
				
				if(op_subq_pk_insert(0, pkptr, OPC_QPOS_TAIL) == OPC_QINS_OK)  //insert queue 0, ok
					op_intrpt_schedule_self(op_sim_time() + op_dist_uniform(0.002), data_code);
				
				else  //insert failure, delete pk
					op_pk_destroy(pkptr);
				}
				FSM_PROFILE_SECTION_OUT (state2_enter_exec)

			/** state (up_arvl) exit executives **/
			FSM_STATE_EXIT_FORCED (2, "up_arvl", "bplc_mac [up_arvl exit execs]")


			/** state (up_arvl) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "up_arvl", "idle", "tr_8", "bplc_mac [up_arvl -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (low_arvl) enter executives **/
			FSM_STATE_ENTER_FORCED (3, "low_arvl", state3_enter_exec, "bplc_mac [low_arvl enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [low_arvl enter execs]", state3_enter_exec)
				{
				pkptr=op_pk_get(instrm_from_phy);
				
				op_pk_nfd_access(pkptr,"Type",&type);
				op_pk_nfd_access(pkptr,"SRC",&src);
				op_pk_nfd_access(pkptr,"DEST",&dest);
				
				distance =op_td_get_dbl (pkptr,OPC_TDA_BU_DISTANCE);
				
				if(distance==0 || src == node_address || (dest != -1 && dest != node_address))  //not give me, myself
				{
					op_pk_destroy (pkptr);
				}
				
				else  //give me and not from myself
				{
					switch(type)
					{
					case 0:  //receive data pk
						op_pk_nfd_get_pkt(pkptr, "DATA", &pktptr);
						
						op_pk_nfd_access(pktptr, "DEST", &dest);
						
						if(dest == node_address)  //data pk to me
						{
							op_pk_send(pktptr, instrm_to_intf);  //send to uplayer
							cco_rev_sta_data_flag=1;
						}
						else   //not to me, forward it, put it in queue 0
						{
							if(op_subq_pk_insert(0, pktptr, OPC_QPOS_TAIL) == OPC_QINS_OK) //put in queue 0
							{
								
								//if(node_address == 7)					
								{
									FILE *fp;
									fp = fopen("..\\mac_foward_data.txt", "a+");
									fprintf(fp, "%d, %f\n", node_address, op_sim_time());  //++mac_forward_nb
									fclose(fp);
									
								}
								
								op_intrpt_schedule_self(op_sim_time() + op_dist_uniform(0.002), data_code);	
							}
							else
								op_pk_destroy(pktptr);
						}
						
						op_pk_destroy(pkptr);
						
						break;
						
					case 1:  //receive CCO's data
						if(node_address==dest)
						{
							//op_pk_send(pkptr, instrm_to_intf);  //send to uplayer
							cco_to_sta_data_flag=1;//sta or pco rcv cco data pk
						}
						break;
						
					case 10:  //receive beacon, send a meter_reading_packet to up_floor
						next_hop[src]=src;
						
						op_pk_nfd_access(pkptr,"hop_count", &i);
						
						if(i >= depth)  //not from parent node, delete pk
							op_pk_destroy(pkptr);
						else   //frome parent node			
						{
							op_pk_nfd_access(pkptr,"beacon_slot",&beacon_slot_time);
							op_pk_nfd_access(pkptr,"csma_slot",&csma_slot_time);
							op_pk_nfd_access(pkptr,"tdma_slot",&tdma_slot_time);
							op_pk_nfd_access(pkptr,"bind_csma_slot",&bind_csma_slot_time);
							
							if(depth > 10)
							{
								depth = i;
								depth++;   //my depth
							}
							else;
							
							if(next_hop[0] < 0)
								next_hop[0] = src;
							else;
							
							if(flag == 0)  //not join net, then request
							{
								//op_pk_send(pkptr,instrm_to_intf);
								pk=op_pk_create_fmt("node_request");
								
								op_pk_nfd_set(pk,"Type",20);
								op_pk_nfd_set(pk,"SRC",node_address);
								op_pk_nfd_set(pk,"DEST",src);
								op_pk_nfd_set(pk,"node type", node_type);  //node type
								op_pk_nfd_set(pk,"hop_count", depth);  //node depth
								op_pk_nfd_set(pk,"NET_SRC",node_address);
								op_pk_nfd_set(pk,"NET_DEST", 0);
								
								if(op_subq_pk_insert(1,pk,OPC_QPOS_TAIL)!=OPC_QINS_OK) //put in queue 1
								{
									op_pk_destroy(pk);
								}
								else;
								
								op_intrpt_schedule_self(op_sim_time()+beacon_slot_time, csma_code); //schedule csma period
								op_intrpt_schedule_self(op_sim_time()+csma_slot_time, tdma_code);
								op_intrpt_schedule_self(op_sim_time()+tdma_slot_time, bind_csma_code);
								
							}
							else  //already join net, schedule periods
							{
								op_intrpt_schedule_self(op_sim_time()+beacon_slot_time, csma_code);
								op_intrpt_schedule_self(op_sim_time()+csma_slot_time, tdma_code);
								op_intrpt_schedule_self(op_sim_time()+tdma_slot_time, bind_csma_code);
								
								if(node_type == 1)  //PCO, maybe rebroadcast
								{
									for(i=0; i<6; i++)
									{
										sprintf(pco_node, "pco_%d", i); //get pco_i field
										op_pk_nfd_get(pkptr, pco_node, &j);
										
										if(j ==  node_address)  //me in beacon
										{
											op_pk_nfd_set(pkptr, "SRC", node_address);
											op_pk_nfd_set(pkptr, "hop_count", depth);
											
											pkptr1  = op_pk_copy(pkptr);
											
											if(op_subq_pk_insert(2, pkptr1, OPC_QPOS_TAIL)!=OPC_QINS_OK)  //put in queue 2
											{
												op_pk_destroy(pkptr1);
											}
											else
												op_intrpt_schedule_self(op_sim_time()+ 0.5*i, pco_send_beacon_code);
											
											break;
										}
										else;
									}
								}
								else;  //STA, do not rebroadcast
							}
							op_pk_destroy(pkptr);
						}
						
						break;
						
					case 20: //receive node's join request
						next_hop[src]=src;
						
						op_pk_nfd_access(pkptr, "NET_SRC", &i);
						next_hop[i]=src;
						
						node_type2 = -1;
						op_pk_nfd_access(pkptr, "node type", &node_type2);
						
						if(node_address == 0 && src != 0 ) //is CCO, reply ack
						{	
							if(node_type2 == 1)  //PCO, CCO records it
							{	
								for(j=0; j<10; j++)
								{
									if(pco_node1[j] == -1)
									{
										pco_node1[j] = i;
										break;
									}
									else;
								}
							}
							else;
							
							//			node_type1[i] = node_type2; //CCO record node's type
							
							pk=op_pk_create_fmt("ack_information");
							op_pk_nfd_set(pk,"Type", 30);
							op_pk_nfd_set(pk,"SRC", 0);
							op_pk_nfd_set(pk,"DEST",src);
							op_pk_nfd_set(pk,"NET_SRC", 0);
							op_pk_nfd_set(pk,"NET_DEST", i);
							
							op_pk_send(pk,instrm_to_phy);
							
							tei++;
							//cco_send_data++;
							cco_data++;
							flag=1;
						}
						else if(node_type == 1)  //is PCO, forward pk upward
						{
							dest = next_hop[0];  //to CCO
							
							op_pk_nfd_set(pkptr, "SRC", node_address);
							op_pk_nfd_set(pkptr, "DEST", dest);
							
							pkptr1  = op_pk_copy(pkptr);
							
							if(op_subq_pk_insert(1, pkptr1, OPC_QPOS_TAIL)!=OPC_QINS_OK)  //put in queue 1
							{
								op_pk_destroy(pkptr1);
							}
							else;
							
							op_intrpt_schedule_self(op_sim_time(), data_code);
						}
						else;
						
						op_pk_destroy(pkptr);
						break;
						
					case 30:  //receive CCO's join ack
						if(node_address==dest)
						{
							op_pk_nfd_access(pkptr, "NET_DEST", &i);
							
							if(i == node_address)  //give me
							{	
								flag=1;  //join net
								op_stat_write(join_net_time_gshandle, node_address);
							}
							else   //not give me, forward it
							{
								if(node_type == 1)  //is PCO, forward pk downward
								{
									dest = next_hop[i];
									
									op_pk_nfd_set(pkptr, "SRC", node_address);
									op_pk_nfd_set(pkptr, "DEST", dest);
									
									pkptr1  = op_pk_copy(pkptr);
									
									if(op_subq_pk_insert(1, pkptr1, OPC_QPOS_TAIL)!=OPC_QINS_OK)  //put in queue 1
									{
										op_pk_destroy(pkptr1);
									}
									else;
									
									op_intrpt_schedule_self(op_sim_time(), data_code);
								}
								else;
							}
							
							op_pk_destroy(pkptr);
						}
						else
							op_pk_destroy(pkptr);
						
						break;
						
					default:  op_pk_destroy(pkptr);
						break;
					}
				}
				
				
				
				
				}
				FSM_PROFILE_SECTION_OUT (state3_enter_exec)

			/** state (low_arvl) exit executives **/
			FSM_STATE_EXIT_FORCED (3, "low_arvl", "bplc_mac [low_arvl exit execs]")


			/** state (low_arvl) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "low_arvl", "idle", "tr_11", "bplc_mac [low_arvl -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (cco_send_beacon) enter executives **/
			FSM_STATE_ENTER_FORCED (4, "cco_send_beacon", state4_enter_exec, "bplc_mac [cco_send_beacon enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [cco_send_beacon enter execs]", state4_enter_exec)
				{
				pkptr  = op_pk_create_fmt ("node0_beacon");
				
				op_pk_nfd_set(pkptr,"DEST", -1);
				op_pk_nfd_set(pkptr,"SRC",node_address);
				op_pk_nfd_set(pkptr,"Type",10);
				op_pk_nfd_set(pkptr,"beacon_slot",5);
				op_pk_nfd_set(pkptr,"csma_slot",15);
				op_pk_nfd_set(pkptr,"tdma_slot",17);
				op_pk_nfd_set(pkptr,"bind_csma_slot",20);
				op_pk_nfd_set(pkptr,"hop_count", 0);
				
				//set pco fields
				
				j = 0; 
				for(i=0; i<10; i++)
					{
					if(pco_node1[i] > 0)
						{
						sprintf(pco_node, "pco_%d", j); //get pco_i field
						
						op_pk_nfd_set(pkptr, pco_node, pco_node1[i]);
						
						j++;
						}
					else;
					}
				
				/*
				j = 0; 
				for(i=0; i<50; i++)
					{
					
					if(node_type1[i] == 1)
						{
						sprintf(pco_node, "pco_%d", j); //get pco_i field
						op_pk_nfd_set(pkptr, pco_node, i);
						
						j++;
						}
					else;
					}
				*/
				op_pk_send(pkptr, instrm_to_phy);
				
				op_intrpt_schedule_self(op_sim_time()+cycle_time, beacon_code);
				
				
					
				
				
				
				}
				FSM_PROFILE_SECTION_OUT (state4_enter_exec)

			/** state (cco_send_beacon) exit executives **/
			FSM_STATE_EXIT_FORCED (4, "cco_send_beacon", "bplc_mac [cco_send_beacon exit execs]")


			/** state (cco_send_beacon) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "cco_send_beacon", "idle", "tr_14", "bplc_mac [cco_send_beacon -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (csma) enter executives **/
			FSM_STATE_ENTER_FORCED (5, "csma", state5_enter_exec, "bplc_mac [csma enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [csma enter execs]", state5_enter_exec)
				{
				csma_key=1;  //open csma period
				
				op_intrpt_schedule_self(op_sim_time() + op_dist_uniform(1.0), data_code);
				
				
				/*
				if((rx_busy==0)&&(csma_key==0))
					{
					if( (!op_subq_empty(1)) || (!op_subq_empty(2))|| (!op_subq_empty(3)) )
						{
						
						t=(int)op_dist_uniform(300);
						time=t*slot_time;//*slot_time;
						
						op_intrpt_schedule_self(op_sim_time()+time,data_code);
						}
					else;
					
					}
				else
					{
					if( (!op_subq_empty(1)) || (!op_subq_empty(2))|| (!op_subq_empty(3)) )
						t=(int)op_dist_uniform(300);
						time=t*slot_time;
						op_intrpt_schedule_self(op_sim_time()+time,data_code);
					}
				*/
				}
				FSM_PROFILE_SECTION_OUT (state5_enter_exec)

			/** state (csma) exit executives **/
			FSM_STATE_EXIT_FORCED (5, "csma", "bplc_mac [csma exit execs]")


			/** state (csma) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "csma", "idle", "tr_16", "bplc_mac [csma -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (tdma) enter executives **/
			FSM_STATE_ENTER_FORCED (6, "tdma", state6_enter_exec, "bplc_mac [tdma enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [tdma enter execs]", state6_enter_exec)
				{
				csma_key=0;  //close csma period
				}
				FSM_PROFILE_SECTION_OUT (state6_enter_exec)

			/** state (tdma) exit executives **/
			FSM_STATE_EXIT_FORCED (6, "tdma", "bplc_mac [tdma exit execs]")


			/** state (tdma) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "tdma", "idle", "tr_18", "bplc_mac [tdma -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (bind_csma) enter executives **/
			FSM_STATE_ENTER_FORCED (7, "bind_csma", state7_enter_exec, "bplc_mac [bind_csma enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [bind_csma enter execs]", state7_enter_exec)
				{
				/*
				if((rx_busy==0)&&(bind_csma_key==0))
					{
					if(!op_subq_empty(0))
						{
						t=(int)op_dist_uniform(4);
						time=t;//*slot_time;
						pkptr=op_subq_pk_remove(0,OPC_QPOS_HEAD);
						op_pk_send(pkptr,instrm_to_phy);
						//cco_send_data++;
						//op_stat_write (cco_send_data_packet_stathandle,cco_send_data);
						//quit bind csma
						op_intrpt_schedule_self(op_sim_time(),quit_bind_csma_code);
						}
					else;
					
					}
				else
					{
					if(!op_subq_empty(0))
						op_intrpt_schedule_self(op_sim_time(),bind_csma_code);
					}
				op_intrpt_schedule_self(op_sim_time(),quit_bind_csma_code);
				bind_csma_key=1;
				*/
				}
				FSM_PROFILE_SECTION_OUT (state7_enter_exec)

			/** state (bind_csma) exit executives **/
			FSM_STATE_EXIT_FORCED (7, "bind_csma", "bplc_mac [bind_csma exit execs]")


			/** state (bind_csma) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "bind_csma", "idle", "tr_22", "bplc_mac [bind_csma -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (rx_busy) enter executives **/
			FSM_STATE_ENTER_FORCED (8, "rx_busy", state8_enter_exec, "bplc_mac [rx_busy enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [rx_busy enter execs]", state8_enter_exec)
				{
				rx_busy=1;//rx busy
				}
				FSM_PROFILE_SECTION_OUT (state8_enter_exec)

			/** state (rx_busy) exit executives **/
			FSM_STATE_EXIT_FORCED (8, "rx_busy", "bplc_mac [rx_busy exit execs]")


			/** state (rx_busy) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "rx_busy", "idle", "tr_26", "bplc_mac [rx_busy -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (rx_idle) enter executives **/
			FSM_STATE_ENTER_FORCED (9, "rx_idle", state9_enter_exec, "bplc_mac [rx_idle enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [rx_idle enter execs]", state9_enter_exec)
				{
				rx_busy=0;//rx idle
				}
				FSM_PROFILE_SECTION_OUT (state9_enter_exec)

			/** state (rx_idle) exit executives **/
			FSM_STATE_EXIT_FORCED (9, "rx_idle", "bplc_mac [rx_idle exit execs]")


			/** state (rx_idle) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "rx_idle", "idle", "tr_24", "bplc_mac [rx_idle -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (send_data) enter executives **/
			FSM_STATE_ENTER_FORCED (10, "send_data", state10_enter_exec, "bplc_mac [send_data enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [send_data enter execs]", state10_enter_exec)
				{
				if(csma_key==1)  //csma periond, can send pk
				{
					if(rx_busy==0)  //channel is idle, send frame
					{
						if (!op_subq_empty(1))
						{
							pkptr=op_subq_pk_remove(1,OPC_QPOS_HEAD);
							op_pk_send(pkptr,instrm_to_phy);
						}
						
						else if(!op_subq_empty(2)) 
						{
							pkptr=op_subq_pk_remove(2,OPC_QPOS_HEAD);
							op_pk_send(pkptr,instrm_to_phy);
						}
						
						else if(!op_subq_empty(3)) 
						{
							pkptr=op_subq_pk_remove(3,OPC_QPOS_HEAD);
							op_pk_send(pkptr,instrm_to_phy);
						}
						
						else if(!op_subq_empty(0))   //data queue, queue 0
						{
							if(flag == 1)  //already join net
							{
								pkptr=op_subq_pk_remove(0, OPC_QPOS_HEAD);
								
								op_pk_nfd_access(pkptr, "DEST", &i);
								
								dest = next_hop[i];				
								
								pk=op_pk_create_fmt("mac_data_packet");
								
								op_pk_nfd_set(pk,"Type", 0);
								op_pk_nfd_set(pk,"SRC", node_address);
								op_pk_nfd_set(pk,"DEST", dest);  //to nexthop
								op_pk_nfd_set_pkt(pk, "DATA", pkptr);
								
								op_pk_send(pk, instrm_to_phy);
								
				//{
				//FILE *fp;
				
				/*
				if(node_address == 5)
					{
					fp = fopen("..\\mac_send_data.txt", "a+");
				fprintf(fp, "%d, %f\n", ++mac_send_nb, op_sim_time());
				fclose(fp);
					}
				
				if(node_address == 4)
					{
					fp = fopen("..\\mac_foward_data.txt", "a+");
				fprintf(fp, "%d, %f\n", ++mac_forward_nb, op_sim_time());
				fclose(fp);
					}*/
				//}
				
								
							}
							else;  //not join net, do not send data pk
						}
						else;
					}
					else;   //channel is busy, do nothing
					
					if((!op_subq_empty(0)) || (!op_subq_empty(1)) || (!op_subq_empty(2))|| (!op_subq_empty(3)))
					{
						op_intrpt_schedule_self(op_sim_time() + op_dist_uniform(0.002), data_code);
					}
					else;
				}
				else;  //non csma period
				}
				FSM_PROFILE_SECTION_OUT (state10_enter_exec)

			/** state (send_data) exit executives **/
			FSM_STATE_EXIT_FORCED (10, "send_data", "bplc_mac [send_data exit execs]")


			/** state (send_data) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "send_data", "idle", "tr_28", "bplc_mac [send_data -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (quit_bind_csma) enter executives **/
			FSM_STATE_ENTER_FORCED (11, "quit_bind_csma", state11_enter_exec, "bplc_mac [quit_bind_csma enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [quit_bind_csma enter execs]", state11_enter_exec)
				{
				bind_csma_key=0;
				
				}
				FSM_PROFILE_SECTION_OUT (state11_enter_exec)

			/** state (quit_bind_csma) exit executives **/
			FSM_STATE_EXIT_FORCED (11, "quit_bind_csma", "bplc_mac [quit_bind_csma exit execs]")


			/** state (quit_bind_csma) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "quit_bind_csma", "idle", "tr_32", "bplc_mac [quit_bind_csma -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (pco_send_beacon) enter executives **/
			FSM_STATE_ENTER_FORCED (12, "pco_send_beacon", state12_enter_exec, "bplc_mac [pco_send_beacon enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [pco_send_beacon enter execs]", state12_enter_exec)
				{
						if (!op_subq_empty(2))
						{
							pkptr=op_subq_pk_remove(2,OPC_QPOS_HEAD);
							op_pk_send(pkptr,instrm_to_phy);
						}
						else;
				
				}
				FSM_PROFILE_SECTION_OUT (state12_enter_exec)

			/** state (pco_send_beacon) exit executives **/
			FSM_STATE_EXIT_FORCED (12, "pco_send_beacon", "bplc_mac [pco_send_beacon exit execs]")


			/** state (pco_send_beacon) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "pco_send_beacon", "idle", "tr_33", "bplc_mac [pco_send_beacon -> idle : default / ]")
				/*---------------------------------------------------------*/



			/** state (level2_send_beacon) enter executives **/
			FSM_STATE_ENTER_FORCED (13, "level2_send_beacon", state13_enter_exec, "bplc_mac [level2_send_beacon enter execs]")
				FSM_PROFILE_SECTION_IN ("bplc_mac [level2_send_beacon enter execs]", state13_enter_exec)
				{
				if(node_address==10)//指定10号节点为第二级PCO
					{
				pkptr  = op_pk_create_fmt ("level1_beacon");
				op_pk_nfd_set(pkptr,"DEST",-1);
				op_pk_nfd_set(pkptr,"SRC",node_address);
				op_pk_nfd_set(pkptr,"Type",21);
				op_pk_send(pkptr, instrm_to_phy);
					}
				else;
				}
				FSM_PROFILE_SECTION_OUT (state13_enter_exec)

			/** state (level2_send_beacon) exit executives **/
			FSM_STATE_EXIT_FORCED (13, "level2_send_beacon", "bplc_mac [level2_send_beacon exit execs]")


			/** state (level2_send_beacon) transition processing **/
			FSM_TRANSIT_FORCE (1, state1_enter_exec, ;, "default", "", "level2_send_beacon", "idle", "tr_35", "bplc_mac [level2_send_beacon -> idle : default / ]")
				/*---------------------------------------------------------*/



			}


		FSM_EXIT (0,"bplc_mac")
		}
	}




void
_op_bplc_mac_diag (OP_SIM_CONTEXT_ARG_OPT)
	{
	/* No Diagnostic Block */
	}




void
_op_bplc_mac_terminate (OP_SIM_CONTEXT_ARG_OPT)
	{

	FIN_MT (_op_bplc_mac_terminate ())


	/* No Termination Block */

	Vos_Poolmem_Dealloc (op_sv_ptr);

	FOUT
	}


/* Undefine shortcuts to state variables to avoid */
/* syntax error in direct access to fields of */
/* local variable prs_ptr in _op_bplc_mac_svar function. */
#undef node_address
#undef dest_addr
#undef node_id
#undef flag
#undef cco_send_data_packet_stathandle
#undef sta_send_data_packet_stathandle
#undef cco_send_data
#undef sta_send_data
#undef rx_busy
#undef csma_key
#undef bind_csma_key
#undef cycle_time
#undef tei
#undef sta_data_stathandle
#undef cco_data_stathandle
#undef level_flag
#undef node_type
#undef next_hop
#undef depth
#undef cco_rev_sta_data_flag
#undef cco_to_sta_data_flag

#undef FIN_PREAMBLE_DEC
#undef FIN_PREAMBLE_CODE

#define FIN_PREAMBLE_DEC
#define FIN_PREAMBLE_CODE

VosT_Obtype
_op_bplc_mac_init (int * init_block_ptr)
	{
	VosT_Obtype obtype = OPC_NIL;
	FIN_MT (_op_bplc_mac_init (init_block_ptr))

	obtype = Vos_Define_Object_Prstate ("proc state vars (bplc_mac)",
		sizeof (bplc_mac_state));
	*init_block_ptr = 0;

	FRET (obtype)
	}

VosT_Address
_op_bplc_mac_alloc (VosT_Obtype obtype, int init_block)
	{
#if !defined (VOSD_NO_FIN)
	int _op_block_origin = 0;
#endif
	bplc_mac_state * ptr;
	FIN_MT (_op_bplc_mac_alloc (obtype))

	ptr = (bplc_mac_state *)Vos_Alloc_Object (obtype);
	if (ptr != OPC_NIL)
		{
		ptr->_op_current_block = init_block;
#if defined (OPD_ALLOW_ODB)
		ptr->_op_current_state = "bplc_mac [init enter execs]";
#endif
		}
	FRET ((VosT_Address)ptr)
	}



void
_op_bplc_mac_svar (void * gen_ptr, const char * var_name, void ** var_p_ptr)
	{
	bplc_mac_state		*prs_ptr;

	FIN_MT (_op_bplc_mac_svar (gen_ptr, var_name, var_p_ptr))

	if (var_name == OPC_NIL)
		{
		*var_p_ptr = (void *)OPC_NIL;
		FOUT
		}
	prs_ptr = (bplc_mac_state *)gen_ptr;

	if (strcmp ("node_address" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->node_address);
		FOUT
		}
	if (strcmp ("dest_addr" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->dest_addr);
		FOUT
		}
	if (strcmp ("node_id" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->node_id);
		FOUT
		}
	if (strcmp ("flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->flag);
		FOUT
		}
	if (strcmp ("cco_send_data_packet_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cco_send_data_packet_stathandle);
		FOUT
		}
	if (strcmp ("sta_send_data_packet_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->sta_send_data_packet_stathandle);
		FOUT
		}
	if (strcmp ("cco_send_data" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cco_send_data);
		FOUT
		}
	if (strcmp ("sta_send_data" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->sta_send_data);
		FOUT
		}
	if (strcmp ("rx_busy" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->rx_busy);
		FOUT
		}
	if (strcmp ("csma_key" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->csma_key);
		FOUT
		}
	if (strcmp ("bind_csma_key" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->bind_csma_key);
		FOUT
		}
	if (strcmp ("cycle_time" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cycle_time);
		FOUT
		}
	if (strcmp ("tei" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->tei);
		FOUT
		}
	if (strcmp ("sta_data_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->sta_data_stathandle);
		FOUT
		}
	if (strcmp ("cco_data_stathandle" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cco_data_stathandle);
		FOUT
		}
	if (strcmp ("level_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->level_flag);
		FOUT
		}
	if (strcmp ("node_type" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->node_type);
		FOUT
		}
	if (strcmp ("next_hop" , var_name) == 0)
		{
		*var_p_ptr = (void *) (prs_ptr->next_hop);
		FOUT
		}
	if (strcmp ("depth" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->depth);
		FOUT
		}
	if (strcmp ("cco_rev_sta_data_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cco_rev_sta_data_flag);
		FOUT
		}
	if (strcmp ("cco_to_sta_data_flag" , var_name) == 0)
		{
		*var_p_ptr = (void *) (&prs_ptr->cco_to_sta_data_flag);
		FOUT
		}
	*var_p_ptr = (void *)OPC_NIL;

	FOUT
	}

