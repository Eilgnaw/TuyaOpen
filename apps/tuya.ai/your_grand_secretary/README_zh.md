# 基于 T5AI-Board的 「内阁首辅」

## 使用方式
### 测试功能
- 配网后,获取到开发板的局域网 IP 地址,例如为 `192.168.31.110`
- 执行 curl 测试是否通讯正常
```
curl -X POST http://192.168.31.110:8080/api/message \
       -H "Content-Type: application/json" \
       -d '{"msg":"奏折:【涂鸦智能】185280，涂鸦智能动态登录验证码。工作人员不会向你索要，请勿向任何人泄露。"}'
```
发送后,开发板将会播报短信内容

### 手机快捷指令配置
- 导入快捷指令 `https://www.icloud.com/shortcuts/c6374ac3282848c383747ede41050bdb`
- 修改快捷指令中 ip 地址为设备的实际地址
- 在快捷指令中创建自动化-信息-信息包含-输入 `【` -立即运行-下一步-选择刚刚导入的快捷指令
- 发送短信 `【涂鸦智能】185280，涂鸦智能动态登录验证码。工作人员不会向你索要，请勿向任何人泄露。` 到手机检查自动化运行效果


## 代码修改

~~### 添加发送文本到智能体~~ 新版 TuyaOpen 已内置发送文本方法
~~- 在 `TuyaOpen/apps/tuya.ai/ai_components/ai_audio/src/ai_audio_agent.c` 中添加发送文本方法~~
~~- 参考代码如下~~
```
/**
 * @brief Send text message to AI service.
 * @param text Pointer to the text message.
 * @param len Length of the text message.
 * @return OPERATE_RET - OPRT_OK on success, or an error code on failure.
 */
OPERATE_RET ai_audio_agent_upload_text(const char *text, uint32_t len)
{
    OPERATE_RET rt = OPRT_OK;

    if (!text || len == 0) {
        PR_ERR("invalid text parameter");
        return OPRT_INVALID_PARM;
    }

    if (!sg_ai.is_online) {
        PR_ERR("ai agent is not online");
        return OPRT_COM_ERROR;
    }

    PR_DEBUG("sending text to AI: %s", text);

    // Start event for text
    memset(sg_ai.event_id, 0, AI_UUID_V4_LEN);
    
    char attr_text_enable[64] = {0};
    snprintf(attr_text_enable, sizeof(attr_text_enable), "{\"processing.interrupt\":true}");

    AI_ATTRIBUTE_T attr[] = {{
        .type = 1003,
        .payload_type = ATTR_PT_STR,
        .length = strlen(attr_text_enable),
        .value.str = attr_text_enable,
    }};
    uint8_t *out = NULL;
    uint32_t out_len = 0;
    tuya_pack_user_attrs(attr, CNTSOF(attr), &out, &out_len);
    rt = tuya_ai_event_start(sg_ai.session_id, sg_ai.event_id, out, out_len);
    tal_free(out);
    if (rt) {
        PR_ERR("start text event failed, rt:%d", rt);
        return rt;
    }

    // Send text data
    AI_BIZ_ATTR_INFO_T text_attr = {
        .flag = AI_HAS_ATTR,
        .type = AI_PT_TEXT,
        .value.text = {
            .session_id_list = NULL,
        },
    };

    AI_BIZ_HEAD_INFO_T head = {
        .stream_flag = AI_STREAM_START,
        .len = len,
    };

    rt = tuya_ai_send_biz_pkt(TY_AI_CHAT_ID_DS_TEXT, &text_attr, AI_PT_TEXT, &head, (char *)text);
    if (rt != OPRT_OK) {
        PR_ERR("send text packet failed, rt:%d", rt);
        return rt;
    }

    // End text stream
    head.stream_flag = AI_STREAM_END;
    head.len = 0;
    rt = tuya_ai_send_biz_pkt(TY_AI_CHAT_ID_DS_TEXT, &text_attr, AI_PT_TEXT, &head, NULL);
    if (rt != OPRT_OK) {
        PR_ERR("end text stream failed, rt:%d", rt);
        return rt;
    }

    // End event
    AI_ATTRIBUTE_T end_attr[] = {{
        .type = 1002,
        .payload_type = ATTR_PT_U16,
        .length = 2,
        .value.u16 = TY_AI_CHAT_ID_DS_TEXT,
    }};
    out = NULL;
    out_len = 0;
    tuya_pack_user_attrs(end_attr, CNTSOF(end_attr), &out, &out_len);
    rt = tuya_ai_event_payloads_end(sg_ai.session_id, sg_ai.event_id, out, out_len);
    tal_free(out);
    if (rt != OPRT_OK) {
        PR_ERR("text payloads end failed, rt:%d", rt);
        return rt;
    }

    rt = tuya_ai_event_end(sg_ai.session_id, sg_ai.event_id, NULL, 0);
    if (rt != OPRT_OK) {
        PR_ERR("text event end failed, rt:%d", rt);
        return rt;
    }

    PR_DEBUG("text sent successfully, event_id:%s", sg_ai.event_id);
    return rt;
}
```

### 修改屏幕显示反向
- 在 `/Users/wxl/Documents/TuyaOpen/boards/T5AI/TUYA_T5AI_BOARD/tuya_t5ai_ex_module.h` 修改 `#define BOARD_LCD_ROTATION   TUYA_DISPLAY_ROTATION_90`




## 项目介绍
### 项目名称
内阁首辅
### 项目背景：
在当今社会，个人手机每天会接收大量短信、邮件与通知。逐一查看、筛选与处理，不仅耗费时间，还容易遗漏关键信息。随着人工智能的发展，我们希望利用 AI 帮助用户预处理这些推送信息，并通过趣味化的方式增强使用体验。

#### 创意来源
项目灵感源自明朝的「内阁制」。在古代，内阁大臣负责处理国家事务，再将关键奏报呈递于皇帝。本项目将 AI 角色设定为「大明内阁首辅」，以臣子奏折、奏对的方式汇报用户的短信与邮件。此设定既合乎身份，又能提供情绪价值，使原本枯燥的系统提示变得有趣、仪式感十足。

### 项目目标
1. 将手机接收的短信信息，自动转发至 T5AI-Board。

2. T5AI-Board 调用涂鸦云智能体中的「内阁首辅」角色，获取处理结果。

3. 将结果通过语音播报，同时在屏幕上播放大臣上奏的动画。

###  硬件清单：
- T5AI 开发板带屏版本
- 外壳使用 `techeditor`制作的外壳 3D 打印 [涂鸦 (Tuya) T5AI开发板外壳 v1.0]("https://makerworld.com.cn/zh/models/1287019-tu-ya-tuya-t5aikai-fa-ban-wai-ke-v1-0#profileId-1379668")
###  核心实现功能
- 开机启动一个 HTTP Server，接收来自手机的短信内容（HTTP POST 请求）。
- 接入涂鸦云智能体
- 使用预设角色「大明内阁首辅」进行语义处理与格式化输出。
- 语音播报与动画展示

## 产品呈现
### 外观设计图/产品照片
### 用户体验流程
- 用户通过 iOS 快捷指令自动将短信发送至 T5AI HTTP Server。
- T5AI 收到内容，转发至涂鸦云智能体。
- 智能体生成「内阁首辅」风格的回复。
- 开发板播报语音，并展示动画。


