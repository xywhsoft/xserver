/* xllm v3 core public API; all 53 externals from lib/xllm/xllm.h.
 * xrt handles stay opaque in the header; the host supplies xrt symbols. */

/* parts & messages */
XS_XLLM_SYMBOL(xllmPartInit)
XS_XLLM_SYMBOL(xllmPartSetText)
XS_XLLM_SYMBOL(xllmPartSetImageData)
XS_XLLM_SYMBOL(xllmPartSetImageUrl)
XS_XLLM_SYMBOL(xllmPartSetAudioData)
XS_XLLM_SYMBOL(xllmPartSetFileData)
XS_XLLM_SYMBOL(xllmPartSetNative)
XS_XLLM_SYMBOL(xllmMessageInit)
XS_XLLM_SYMBOL(xllmMessageUnit)
XS_XLLM_SYMBOL(xllmMessageSetContent)
XS_XLLM_SYMBOL(xllmMessageSetReasoning)
XS_XLLM_SYMBOL(xllmMessageSetToolCallId)
XS_XLLM_SYMBOL(xllmMessageAddToolCall)
XS_XLLM_SYMBOL(xllmMessageAddPart)
XS_XLLM_SYMBOL(xllmMessageSetNative)
XS_XLLM_SYMBOL(xllmMessageFromResponse)

/* requests & responses */
XS_XLLM_SYMBOL(xllmRequestInit)
XS_XLLM_SYMBOL(xllmRequestUnit)
XS_XLLM_SYMBOL(xllmRequestSetModel)
XS_XLLM_SYMBOL(xllmRequestSetReasoningEffort)
XS_XLLM_SYMBOL(xllmRequestSetStop)
XS_XLLM_SYMBOL(xllmRequestSetExtraBody)
XS_XLLM_SYMBOL(xllmRequestSetCancel)
XS_XLLM_SYMBOL(xllmRequestSetDeadline)
XS_XLLM_SYMBOL(xllmRequestSetToolChoice)
XS_XLLM_SYMBOL(xllmRequestAddMessage)
XS_XLLM_SYMBOL(xllmRequestAddTextMessage)
XS_XLLM_SYMBOL(xllmRequestAddToolResult)
XS_XLLM_SYMBOL(xllmRequestAddTool)
XS_XLLM_SYMBOL(xllmResponseDestroy)

/* token estimate & model profiles */
XS_XLLM_SYMBOL(xllmEstimateTextTokens)
XS_XLLM_SYMBOL(xllmEstimateMessageTokens)
XS_XLLM_SYMBOL(xllmModelProfileInit)
XS_XLLM_SYMBOL(xllmModelProfileBuiltin)
XS_XLLM_SYMBOL(xllmModelProfileValidate)
XS_XLLM_SYMBOL(xllmModelProfileSupports)
XS_XLLM_SYMBOL(xllmModelProfileValidateRequest)

/* errors & diagnostics */
XS_XLLM_SYMBOL(xllmErrorInit)
XS_XLLM_SYMBOL(xllmErrorCodeName)
XS_XLLM_SYMBOL(xllmFinishReasonName)
XS_XLLM_SYMBOL(xllmErrorRetryable)

/* client & calls */
XS_XLLM_SYMBOL(xllmClientConfigInit)
XS_XLLM_SYMBOL(xllmClientCreate)
XS_XLLM_SYMBOL(xllmClientDestroy)
XS_XLLM_SYMBOL(xllmClientGetModelProfile)
XS_XLLM_SYMBOL(xllmClientStart)
XS_XLLM_SYMBOL(xllmCallFuture)
XS_XLLM_SYMBOL(xllmCallWait)
XS_XLLM_SYMBOL(xllmCallCancel)
XS_XLLM_SYMBOL(xllmCallDestroy)
XS_XLLM_SYMBOL(xllmClientComplete)
XS_XLLM_SYMBOL(xllmClientBuildRequestJson)

/* allocator */
XS_XLLM_SYMBOL(xllmFree)
