sdk_generate_library(rwip)
sdk_add_include_directories(
    E:/codec/bf_rwip/extinc
    E:/codec/bf_rwip/reg_h
)

set(rw_src E:/codec/bf_rwip/CEVA_BT5.2/rw-btdm-blehost-sw-v11_0_3/src)
set(rwbt_src ${rw_src}/ip/bt/src)
set(rwble_src ${rw_src}/ip/ble/ll/src)

sdk_add_include_directories(
	${rw_src}/ip/em/api
	${rw_src}/ip/ble/ll/src
	${rw_src}/ip/ble/ll/api
    ${rw_src}/ip/ble/iso/data_path
    ${rw_src}/ip/ble/iso/data_path/isogen/api
    ${rw_src}/ip/ble/iso/data_path/isoohci/api
	${rw_src}/ip/bt/src
	${rw_src}/ip/bt/api
	${rw_src}/ip/hci/api
	${rw_src}/ip/host/hl/inc
	${rw_src}/ip/host/hl/api
    ${rw_src}/ip/sch/api
    ${rw_src}/modules/ecc_p256/api
    ${rw_src}/modules/rwip/api
    ${rw_src}/modules/common/api
    ${rw_src}/modules/ke/api
    ${rw_src}/modules/dbg/api
    ${rw_src}/modules/aes/api
    ${rw_src}/modules/nvds/api
    ${rw_src}/modules/h4tl/api
    ${rw_src}/modules/rf/api
)    
#CFG_CLK_ACC
#CFG_BT_PING
#CFG_BT_HCI_TEST_MODE
#CFG_BT_POWER_CONTROL
#CFG_LR
#CFG_SBCGEN
#CFG_SEC_CON
#CFG_BLE_ENABLE

#CFG_OS
#CFG_RSWITCH
#CFG_PHY_UPDATE
#CFG_EM_SIZE
#CFG_LE_PING

#CFG_SNIFF
#CFG_BLE_PDS
#CFG_BLE_EMB
#CFG_ACL
#CFG_BLE_TX_BUFF_DATA
#CFG_BLE_STACK_DBG_PRINT
#CFG_CHNL_ASSESS
#CFG_COMP_ID
#CFG_TEST_MODE

sdk_add_compile_definitions(-DCFG_PERIPHERAL)
sdk_add_compile_definitions(-DCFG_CENTRAL)
sdk_add_compile_definitions(-DCFG_BROADCASTER)
sdk_add_compile_definitions(-DCFG_OBSERVER)

sdk_add_compile_definitions(-DCFG_CSB)
sdk_add_compile_definitions(-DCFG_PCA)

sdk_add_compile_definitions(-DCFG_WLAN_COEX)

sdk_add_compile_definitions(-DCFG_BT)
sdk_add_compile_definitions(-DCFG_BLE)
sdk_add_compile_definitions(-DCFG_EMB)

sdk_add_compile_definitions(-DCFG_HCITL)
sdk_add_compile_definitions(-DCFG_VOHCI)
sdk_add_compile_definitions(-DCFG_ISOOHCI)


sdk_add_compile_definitions(-DCFG_CON_SCO=2)
sdk_add_compile_definitions(-DCFG_CON_ACL=10)
sdk_add_compile_definitions(-DCFG_ISO_CON=3)
#sdk_add_compile_definitions(-DCFG_CON=100)
sdk_add_compile_definitions(-DCFG_RAL=10)
sdk_add_compile_definitions(-DCFG_ACT=10)

sdk_add_compile_definitions(-DCFG_ISOGEN)
sdk_add_compile_definitions(-DCFG_CIS)
sdk_add_compile_definitions(-DCFG_BIS)

sdk_add_compile_definitions(-DCFG_NVDS)
sdk_add_compile_definitions(-DCFG_MODEM_LR)
sdk_add_compile_definitions(-DCFG_STATIC)
sdk_add_compile_definitions(-DCFG_ALLROLES)
sdk_add_compile_definitions(-DCFG_LE_PWR_CTRL)
sdk_add_compile_definitions(-DCFG_BT_PWR_CTRL)
#sdk_add_compile_definitions(-DCFG_CON_CTE_REQ)
#sdk_add_compile_definitions(-DCFG_CON_CTE_RSP)
#sdk_add_compile_definitions(-DCFG_CONLESS_CTE_TX)
#sdk_add_compile_definitions(-DCFG_CONLESS_CTE_RX)
#sdk_add_compile_definitions(-DCFG_AOD)
#sdk_add_compile_definitions(-DCFG_AOA)


sdk_add_compile_definitions(-DCFG_ECC_P256_SUPPORT)
sdk_add_compile_definitions(-DCFG_DBG)

set(rwbt
    ${rwbt_src}/co/rwbt.c
    ${rwbt_src}/co/bt_util_buf.c
    ${rwbt_src}/co/bt_util_lmp.c
    ${rwbt_src}/co/bt_util_key.c
    ${rwbt_src}/co/bt_util_sp.c
    ${rwbt_src}/lm/lm.c
    ${rwbt_src}/lm/lm_task.c
    ${rwbt_src}/lc/lc_sniff.c
    ${rwbt_src}/lc/lc_sco.c
    ${rwbt_src}/lc/lm_sco.c
    ${rwbt_src}/lc/lc.c
    ${rwbt_src}/lc/lc_task.c
    ${rwbt_src}/lc/lc_util.c
    ${rwbt_src}/lc/lc_lmppdu.c
    ${rwbt_src}/lc/lc_clk.c
    ${rwbt_src}/lb/lb.c
    ${rwbt_src}/lb/lb_task.c
    ${rwbt_src}/ld/ld.c
    ${rwbt_src}/ld/ld_inq.c
    ${rwbt_src}/ld/ld_iscan.c
    ${rwbt_src}/ld/ld_page.c
    ${rwbt_src}/ld/ld_pscan.c
    ${rwbt_src}/ld/ld_acl.c
    ${rwbt_src}/ld/ld_bcst.c
    ${rwbt_src}/ld/ld_csb_tx.c
    ${rwbt_src}/ld/ld_csb_rx.c
    ${rwbt_src}/ld/ld_sscan.c
    ${rwbt_src}/ld/ld_strain.c
    ${rwbt_src}/ld/ld_pca.c
    ${rwbt_src}/ld/ld_util.c
)
set(rwble
    ${rwble_src}/co/rwble.c
    ${rwble_src}/co/ble_util.c
    ${rwble_src}/co/ble_util_buf.c
    ${rwble_src}/llm/llm.c
    ${rwble_src}/llm/llm_task.c
    ${rwble_src}/llm/llm_hci.c
    ${rwble_src}/llm/llm_adv.c
    ${rwble_src}/llm/llm_scan.c
    ${rwble_src}/llm/llm_init.c
    ${rwble_src}/llm/llm_test.c
    ${rwble_src}/lli/lli.c
    ${rwble_src}/lli/lli_task.c
    ${rwble_src}/lli/lli_data_path.c
    ${rwble_src}/lli/lli_test.c
    ${rwble_src}/lli/lli_bi.c
    ${rwble_src}/lli/lli_ci.c
    ${rwble_src}/lli/lli_am0.c
    ${rwble_src}/llc/llc.c
    ${rwble_src}/llc/llc_hci.c
    ${rwble_src}/llc/llc_task.c
    ${rwble_src}/llc/llc_llcp.c
    ${rwble_src}/llc/llc_disconnect.c
    ${rwble_src}/llc/llc_ver_exch.c
    ${rwble_src}/llc/llc_encrypt.c
    ${rwble_src}/llc/llc_le_ping.c
    ${rwble_src}/llc/llc_feat_exch.c
    ${rwble_src}/llc/llc_dl_upd.c
    ${rwble_src}/llc/llc_con_upd.c
    ${rwble_src}/llc/llc_chmap_upd.c
    ${rwble_src}/llc/llc_phy_upd.c
    ${rwble_src}/llc/llc_cte.c
    ${rwble_src}/llc/llc_past.c
    ${rwble_src}/llc/llc_clk_acc.c
    ${rwble_src}/llc/llc_dbg.c
    ${rwble_src}/llc/llc_cis.c
    ${rwble_src}/llc/llc_pwr.c
    ${rwble_src}/lld/lld.c
    ${rwble_src}/lld/lld_adv.c
    ${rwble_src}/lld/lld_per_adv.c
    ${rwble_src}/lld/lld_scan.c
    ${rwble_src}/lld/lld_sync.c
    ${rwble_src}/lld/lld_test.c
    ${rwble_src}/lld/lld_init.c
    ${rwble_src}/lld/lld_con.c
    ${rwble_src}/lld/lld_iso.c
    ${rwble_src}/lld/lld_isoal.c
    ${rwble_src}/lld/lld_ci.c
    ${rwble_src}/lld/lld_bi.c
)
file(GLOB_RECURSE tmp_files ${rw_src}/modules/h4tl/*.c)
list (APPEND misc ${tmp_files})

file(GLOB_RECURSE tmp_files ${rw_src}/modules/rwip/*.c)
list (APPEND misc ${tmp_files})

file(GLOB_RECURSE tmp_files ${rw_src}/modules/ecc_p256/*.c)
list (APPEND misc ${tmp_files})

file(GLOB_RECURSE tmp_files ${rw_src}/modules/common/*.c)
list (APPEND misc ${tmp_files})

file(GLOB_RECURSE tmp_files ${rw_src}/modules/ke/*.c)
list (APPEND misc ${tmp_files})

file(GLOB_RECURSE tmp_files ${rw_src}/modules/dbg/*.c)
list (APPEND misc ${tmp_files})

file(GLOB_RECURSE tmp_files ${rw_src}/modules/aes/*.c)
list (APPEND misc ${tmp_files})

file(GLOB_RECURSE tmp_files ${rw_src}/ip/sch/*.c)
list (APPEND misc ${tmp_files})


file(GLOB_RECURSE tmp_files ${rw_src}/ip/hci/*.c)
list (APPEND misc ${tmp_files})

file(GLOB_RECURSE tmp_files ${rw_src}/ip/ble/iso/*.c)
list (APPEND misc ${tmp_files})

list (APPEND misc E:/codec/bf_rwip/extsrc/em_size.c)#for check
sdk_library_add_sources(${rwble} ${rwbt} ${misc})