MIL_3_Tfile_Hdr_ 145A 140A modeler 9 5D72F4C9 5D7632CD 2 7FPCYKRFROZNOYF Zhi@Ren 0 0 none none 0 0 none DED86AFA 1576 0 0 0 0 0 0 1d79 5                                                                                                                                                                                                                                                                                                                                                                                        ��g�      @  �  �  b  �  �  �  �  �    =  n  r  �      Type of Service    �������    ����   0          Best Effort (0)          ����          ����         Best Effort (0)       ����      Background (1)      ����      Standard (2)      ����      Excellent Effort (3)      ����      Streaming Multimedia (4)      ����      Interactive Multimedia (5)      ����      Interactive Voice (6)      ����      Reserved (7)      ����         �Indicates the type of service (TOS) classification assigned to the data packets generated by this station. Currently, this information is not used by WLAN MACs.�Z                 	   begsim intrpt         
   ����   
   doc file            	nd_module      endsim intrpt             ����      failure intrpts            disabled      intrpt interval         ԲI�%��}����      priority              ����      recovery intrpts            disabled      subqueue         
            count    ���   
   ����   
      list   	���   
          
   
   super priority             ����            begsim intrpt      begsim intrpt����   ����           ����          ����          ����                        int	\node_address;       int	\dest_addr;       Objid	\node_id;       
int	\flag;          //Packet*		packet;   Packet*		pkptr;           #define	instrm_from_routing    0   #define	instrm_from_proc		1   #define	instrm_to_routing	    1   #define	instrm_to_proc		0   A#define	 FS_TIMES				0.002     //frame space times 60 microsecond   c#define UP_ARVL 			(op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == instrm_from_routing)   a#define LOW_ARVL 			(op_intrpt_type() == OPC_INTRPT_STRM && op_intrpt_strm() == instrm_from_proc)   'enum NodeAttributes //node's attributes   {   	CCO,    	PCO,    	STA   };   ,typedef enum MAC_frame_type //MAC frame type   {   	CCO_beacon,   	PCO_beacon,       STA_beacon   }MAC_frame_type;   (typedef struct beacon_slot_table_element   {	   	int node_TEI;   	int beacon_num;   	int node_level;   }beacon_slot_table_element;   &typedef struct TDMA_slot_table_element   {	   	int node_TEI;   	int TDMA_num;   }TDMA_slot_table_element;                                                 Z            
   init   
       
      'node_id = op_topo_parent(op_id_self());   Zop_ima_obj_attr_get(node_id,"destination_address",&dest_addr);//to get dest from attribute   :op_ima_obj_attr_get(node_id,"node_address",&node_address);   flag=0;   
                     
   ����   
          pr_state        J            
   idle   
                                       ����             pr_state        J   Z          
   up_arvl   
       
      %pkptr=op_pk_get(instrm_from_routing);   !op_pk_send(pkptr,instrm_to_proc);       
                     
   ����   
          pr_state        J  �          
   low_arvl   
       
      "pkptr=op_pk_get(instrm_from_proc);   $op_pk_send(pkptr,instrm_to_routing);           
                     
   ����   
          pr_state                     �  
      g  
  ;  
          
   tr_7   
       ����          ����          
    ����   
          ����                       pr_transition              ,   �     @   V     �  B            
   tr_8   
       ����          ����          
    ����   
          ����                       pr_transition      	        Y   �     X   �  w   �  Q   X          
   tr_9   
       
   UP_ARVL   
       ����          
    ����   
          ����                       pr_transition      
          g     D      �  B  �          
   tr_10   
       
   LOW_ARVL   
       ����          
    ����   
          ����                       pr_transition              T  x     W  �  q  z  U            
   tr_11   
       
����   
       ����          
    ����   
          ����                       pr_transition              m   �     V    �   �  �  .  O            
   tr_12   
       
   default   
       ����          
    ����   
          ����                       pr_transition                   outstat   d           normal   linear        ԲI�%��}          oms_auto_addr_support   oms_pr   oms_tan           (   General Process Description:    ----------------------------        �The wlan_mac_higher_layer_intf process model accepts packets from any number of sources and discards them regardless of their content or format.               ICI Interfaces:    --------------        None                Packet Formats:    ---------------        None                Statistic Wires:    ----------------        None                Process Registry:    -----------------        Not Applicable               Restrictions:    -------------        None            