/* xmail public API: umbrella xmail.h + xrt/mail*.h, all XRT_API externals */

/* mail */
XS_XMAIL_SYMBOL(xrtMailBoundaryValid)
XS_XMAIL_SYMBOL(xrtMailCrlf)
XS_XMAIL_SYMBOL(xrtMailCrlfWrite)

/* mail_address */
XS_XMAIL_SYMBOL(xrtMailAddress)
XS_XMAIL_SYMBOL(xrtMailAddressCursorInit)
XS_XMAIL_SYMBOL(xrtMailAddressList)
XS_XMAIL_SYMBOL(xrtMailAddressListWrite)
XS_XMAIL_SYMBOL(xrtMailAddressNext)
XS_XMAIL_SYMBOL(xrtMailAddressValid)
XS_XMAIL_SYMBOL(xrtMailAddressWrite)

/* mail_build */
XS_XMAIL_SYMBOL(xrtMailBuilderAddressHeader)
XS_XMAIL_SYMBOL(xrtMailBuilderBody)
XS_XMAIL_SYMBOL(xrtMailBuilderFinish)
XS_XMAIL_SYMBOL(xrtMailBuilderHeader)
XS_XMAIL_SYMBOL(xrtMailBuilderHeaderBlock)
XS_XMAIL_SYMBOL(xrtMailBuilderHeadersEnd)
XS_XMAIL_SYMBOL(xrtMailBuilderInit)
XS_XMAIL_SYMBOL(xrtMailBuilderMultipart)
XS_XMAIL_SYMBOL(xrtMailBuilderPartBegin)
XS_XMAIL_SYMBOL(xrtMailBuilderWordHeader)

/* mail_charset */
XS_XMAIL_SYMBOL(xrtMailCharsetSupported)
XS_XMAIL_SYMBOL(xrtMailCharsetToUtf8)
XS_XMAIL_SYMBOL(xrtMailCharsetToUtf8Write)

/* mail_codec */
XS_XMAIL_SYMBOL(xrtMailBase64)
XS_XMAIL_SYMBOL(xrtMailBase64Decode)
XS_XMAIL_SYMBOL(xrtMailBase64DecodeWrite)
XS_XMAIL_SYMBOL(xrtMailBase64Write)
XS_XMAIL_SYMBOL(xrtMailQp)
XS_XMAIL_SYMBOL(xrtMailQpDecode)
XS_XMAIL_SYMBOL(xrtMailQpDecodeWrite)
XS_XMAIL_SYMBOL(xrtMailQpWrite)

/* mail_compose */
XS_XMAIL_SYMBOL(xrtMailCompose)
XS_XMAIL_SYMBOL(xrtMailComposeWrite)
XS_XMAIL_SYMBOL(xrtMailMessageInit)
XS_XMAIL_SYMBOL(xrtMailMessageValid)

/* mail_date */
XS_XMAIL_SYMBOL(xrtMailDate)
XS_XMAIL_SYMBOL(xrtMailDateParse)
XS_XMAIL_SYMBOL(xrtMailDateWrite)

/* mail_header */
XS_XMAIL_SYMBOL(xrtMailHeader)
XS_XMAIL_SYMBOL(xrtMailHeaderCursorInit)
XS_XMAIL_SYMBOL(xrtMailHeaderNameValid)
XS_XMAIL_SYMBOL(xrtMailHeaderNext)
XS_XMAIL_SYMBOL(xrtMailHeaderUnfold)
XS_XMAIL_SYMBOL(xrtMailHeaderUnfoldWrite)
XS_XMAIL_SYMBOL(xrtMailHeaderValueValid)
XS_XMAIL_SYMBOL(xrtMailHeaderWrite)

/* mail_id */
XS_XMAIL_SYMBOL(xrtMailBoundary)
XS_XMAIL_SYMBOL(xrtMailBoundaryWrite)
XS_XMAIL_SYMBOL(xrtMailMessageId)
XS_XMAIL_SYMBOL(xrtMailMessageIdParse)
XS_XMAIL_SYMBOL(xrtMailMessageIdWrite)

/* mail_message */
XS_XMAIL_SYMBOL(xrtMailMessageBody)
XS_XMAIL_SYMBOL(xrtMailMessageBodyWrite)
XS_XMAIL_SYMBOL(xrtMailMessageHeader)
XS_XMAIL_SYMBOL(xrtMailMessageParse)
XS_XMAIL_SYMBOL(xrtMailMessageTransfer)
XS_XMAIL_SYMBOL(xrtMailTransferParse)

/* mail_multipart */
XS_XMAIL_SYMBOL(xrtMailMultipartCursorInit)
XS_XMAIL_SYMBOL(xrtMailMultipartMarkWrite)
XS_XMAIL_SYMBOL(xrtMailMultipartNext)

/* mail_net */
XS_XMAIL_SYMBOL(xrtMailNetConfigInit)
XS_XMAIL_SYMBOL(xrtMailNetConfigValid)

/* mail_param */
XS_XMAIL_SYMBOL(xrtMailDispositionParse)
XS_XMAIL_SYMBOL(xrtMailMediaTypeParse)
XS_XMAIL_SYMBOL(xrtMailParam)
XS_XMAIL_SYMBOL(xrtMailParamCursorInit)
XS_XMAIL_SYMBOL(xrtMailParamDecodeWrite)
XS_XMAIL_SYMBOL(xrtMailParamFind)
XS_XMAIL_SYMBOL(xrtMailParamFindWrite)
XS_XMAIL_SYMBOL(xrtMailParamNext)
XS_XMAIL_SYMBOL(xrtMailParamWrite)

/* mail_tree */
XS_XMAIL_SYMBOL(xrtMailTreeFree)
XS_XMAIL_SYMBOL(xrtMailTreeLimitsInit)
XS_XMAIL_SYMBOL(xrtMailTreeLimitsValid)
XS_XMAIL_SYMBOL(xrtMailTreeParse)

/* mail_wire */
XS_XMAIL_SYMBOL(xrtMailDot)
XS_XMAIL_SYMBOL(xrtMailDotDecode)
XS_XMAIL_SYMBOL(xrtMailDotDecodeWrite)
XS_XMAIL_SYMBOL(xrtMailDotLine)
XS_XMAIL_SYMBOL(xrtMailDotWrite)
XS_XMAIL_SYMBOL(xrtMailDotWriterFinish)
XS_XMAIL_SYMBOL(xrtMailDotWriterInit)
XS_XMAIL_SYMBOL(xrtMailDotWriterWrite)
XS_XMAIL_SYMBOL(xrtMailLineRead)

/* mail_word */
XS_XMAIL_SYMBOL(xrtMailWordDecode)
XS_XMAIL_SYMBOL(xrtMailWordDecodeWrite)
XS_XMAIL_SYMBOL(xrtMailWordEncode)
XS_XMAIL_SYMBOL(xrtMailWordEncodeWrite)
XS_XMAIL_SYMBOL(xrtMailWordParse)
