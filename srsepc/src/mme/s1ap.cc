/**
 *
 * \section COPYRIGHT
 *
 * Copyright 2013-2017 Software Radio Systems Limited
 *
 * \section LICENSE
 *
 * This file is part of srsLTE.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include "srslte/common/bcd_helpers.h"
#include "mme/s1ap.h"

namespace srsepc{

s1ap::s1ap():
  m_s1mme(-1)
{
}

s1ap::~s1ap()
{
}

int
s1ap::init(s1ap_args_t s1ap_args, srslte::log *s1ap_log)
{
  
  m_s1ap_args = s1ap_args;

  srslte::s1ap_mccmnc_to_plmn(s1ap_args.mcc, s1ap_args.mnc, &m_plmn);

  m_s1ap_log = s1ap_log;

  m_s1mme = enb_listen();
  
  return 0;
}

void
s1ap::stop()
{
  if (m_s1mme != -1){
    close(m_s1mme);
  }
  std::map<uint16_t,enb_ctx_t*>::iterator it = m_active_enbs.begin();
  while(it!=m_active_enbs.end())
  {
    print_enb_ctx_info(*it->second);
    delete it->second;
    m_active_enbs.erase(it++);
  }
  return;
}

int
s1ap::get_s1_mme()
{
  return m_s1mme;
}

int
s1ap::enb_listen()
{
  /*This function sets up the SCTP socket for eNBs to connect to*/
  int sock_fd, err;
  struct sockaddr_in s1mme_addr;
  struct sctp_event_subscribe evnts;

  m_s1ap_log->console("Initializing S1-MME\n");
  sock_fd = socket (AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
  if (sock_fd == -1){
    m_s1ap_log->console("Could not create SCTP socket\n"); 
    return -1;
  }

  //Sets the data_io_event to be able to use sendrecv_info
  bzero (&evnts, sizeof (evnts)) ;
  evnts.sctp_data_io_event = 1;
  if(setsockopt(sock_fd, IPPROTO_SCTP, SCTP_EVENTS, &evnts, sizeof (evnts))){
    m_s1ap_log->console("Subscribing to sctp_data_io_events failed\n");
    return -1;
  }

  //S1-MME bind
  bzero(&s1mme_addr, sizeof(s1mme_addr));
  s1mme_addr.sin_family = AF_INET; 
  inet_pton(AF_INET, m_s1ap_args.mme_bind_addr.c_str(), &(s1mme_addr.sin_addr) );
  s1mme_addr.sin_port = htons(S1MME_PORT);
  err = bind(sock_fd, (struct sockaddr*) &s1mme_addr, sizeof (s1mme_addr));
  if (err != 0){
    m_s1ap_log->error("Error binding SCTP socket\n");
    m_s1ap_log->console("Error binding SCTP socket\n");
    return -1;
  }

  //Listen for connections
  err = listen(sock_fd,SOMAXCONN);
  if (err != 0){
    m_s1ap_log->error("Error in SCTP socket listen\n");
    m_s1ap_log->console("Error in SCTP socket listen\n");
    return -1;
  }

  return sock_fd;
}

bool
s1ap::handle_s1ap_rx_pdu(srslte::byte_buffer_t *pdu, struct sctp_sndrcvinfo *enb_sri) 
{
  LIBLTE_S1AP_S1AP_PDU_STRUCT rx_pdu;

  if(liblte_s1ap_unpack_s1ap_pdu((LIBLTE_BYTE_MSG_STRUCT*)pdu, &rx_pdu) != LIBLTE_SUCCESS) {
    m_s1ap_log->error("Failed to unpack received PDU\n");
    return false;
  }

  switch(rx_pdu.choice_type) {
  case LIBLTE_S1AP_S1AP_PDU_CHOICE_INITIATINGMESSAGE:
    m_s1ap_log->console("Received initiating PDU\n");
    return handle_initiating_message(&rx_pdu.choice.initiatingMessage, enb_sri);
    break;
  case LIBLTE_S1AP_S1AP_PDU_CHOICE_SUCCESSFULOUTCOME:
    m_s1ap_log->console("Received Succeseful Outcome PDU\n");
    return true;//handle_successfuloutcome(&rx_pdu.choice.successfulOutcome);
    break;
  case LIBLTE_S1AP_S1AP_PDU_CHOICE_UNSUCCESSFULOUTCOME:
    m_s1ap_log->console("Received Unsucceseful Outcome PDU\n");
    return true;//handle_unsuccessfuloutcome(&rx_pdu.choice.unsuccessfulOutcome);
    break;
  default:
    m_s1ap_log->console("Unhandled PDU type %d\n", rx_pdu.choice_type);
    return false;
  }

  return true;

}

bool 
s1ap::handle_initiating_message(LIBLTE_S1AP_INITIATINGMESSAGE_STRUCT *msg,  struct sctp_sndrcvinfo *enb_sri)
{
  switch(msg->choice_type) {
  case LIBLTE_S1AP_INITIATINGMESSAGE_CHOICE_S1SETUPREQUEST:
    m_s1ap_log->info("Received S1 Setup Request.\n");
    return handle_s1_setup_request(&msg->choice.S1SetupRequest, enb_sri);
  default:
    m_s1ap_log->error("Unhandled intiating message: %s\n", liblte_s1ap_initiatingmessage_choice_text[msg->choice_type]);
  }
  return true;
}

bool 
s1ap::handle_s1_setup_request(LIBLTE_S1AP_MESSAGE_S1SETUPREQUEST_STRUCT *msg, struct sctp_sndrcvinfo *enb_sri)
{
  
  std::string mnc_str, mcc_str;
  enb_ctx_t enb_ctx;
  srslte::byte_buffer_t reply_msg;
  LIBLTE_S1AP_S1AP_PDU_STRUCT reply_pdu;

  if(!m_s1ap_mngmt_proc.unpack_s1_setup_request(msg, &enb_ctx))
  {
    m_s1ap_log->error("Malformed S1 Setup Request\n");
    return false;
  }

  //Log S1 Setup Request Info
  print_enb_ctx_info(enb_ctx);
  
  //Check matching PLMNs  
  if(enb_ctx.plmn!=m_plmn){
    m_s1ap_log->console("S1 Setup Failure - Unkown PLMN\n");
    m_s1ap_log->info("S1 Setup Failure - Unkown PLMN\n");
    m_s1ap_mngmt_proc.pack_s1_setup_failure(LIBLTE_S1AP_CAUSEMISC_UNKNOWN_PLMN,&reply_msg);
  }
  else{
    std::map<uint16_t,enb_ctx_t*>::iterator it = m_active_enbs.find(enb_ctx.enb_id);
    if(it != m_active_enbs.end())
    {
      //eNB already registered
      //TODO replace enb_ctx
    }
    else
    {
      //new eNB
      enb_ctx_t *enb_ptr = new enb_ctx_t;//TODO use buffer pool here?
      memcpy(enb_ptr,&enb_ctx,sizeof(enb_ctx));
      m_active_enbs.insert(std::pair<uint16_t,enb_ctx_t*>(enb_ptr->enb_id,enb_ptr));
    }
        
    //m_active_enbs.insert(std::pair<uint16_t,enb_ctx_t>(enb_ctx.enb_id,enb_ctx));
    m_s1ap_mngmt_proc.pack_s1_setup_response(m_s1ap_args, &reply_msg);
    m_s1ap_log->console("S1 Setup Response\n");
    m_s1ap_log->info("S1 Setup Response\n");
  }
  
  //Send Reply to eNB
  ssize_t n_sent = sctp_send(m_s1mme,reply_msg.msg, reply_msg.N_bytes, enb_sri, 0);
  if(n_sent == -1)
  {
    m_s1ap_log->console("Failed to send S1 Setup Setup Reply");
    return false;
  }
  
  return true;
  
}

void
s1ap::print_enb_ctx_info(const enb_ctx_t &enb_ctx)
{
  std::string mnc_str, mcc_str;

  if(enb_ctx.enb_name_present)
  {
    m_s1ap_log->console("S1 Setup Request - eNB Name: %s, eNB id: 0x%x\n", enb_ctx.enb_name, enb_ctx.enb_id);
    m_s1ap_log->info("S1 Setup Request - eNB Name: %s, eNB id: 0x%x\n", enb_ctx.enb_name, enb_ctx.enb_id);
  }
  else
  {
    m_s1ap_log->console("S1 Setup Request - eNB Id 0x%x\n", enb_ctx.enb_id);
    m_s1ap_log->info("S1 Setup request - eNB Id 0x%x\n", enb_ctx.enb_id);
  }
  srslte::mcc_to_string(enb_ctx.mcc, &mcc_str);
  srslte::mnc_to_string(enb_ctx.mnc, &mnc_str);
  m_s1ap_log->info("S1 Setup Request - MCC:%s, MNC:%s, PLMN: %d\n", mcc_str.c_str(), mnc_str.c_str(), enb_ctx.plmn);
  m_s1ap_log->console("S1 Setup Request - MCC:%s, MNC:%s, PLMN: %d\n", mcc_str.c_str(), mnc_str.c_str(), enb_ctx.plmn);
  for(int i=0;i<enb_ctx.nof_supported_ta;i++)
  {
    for(int j=0;i<enb_ctx.nof_supported_ta;i++)
    {
      m_s1ap_log->info("S1 Setup Request - TAC %d, B-PLMN %d\n",enb_ctx.tac[i],enb_ctx.bplmns[i][j]);
      m_s1ap_log->console("S1 Setup Request - TAC %d, B-PLMN %d\n",enb_ctx.tac[i],enb_ctx.bplmns[i][j]);
    }
  }
  m_s1ap_log->console("S1 Setup Request - Paging DRX %d\n",enb_ctx.drx);
  return;
}

} //namespace srsepc
