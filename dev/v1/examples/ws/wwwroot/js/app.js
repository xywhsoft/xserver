(function(){
	var hSocket = null;
	var sSelfID = '';
	var arrPeers = [];
	var objStatus = document.getElementById('StatusText');
	var objSelf = document.getElementById('SelfText');
	var objLog = document.getElementById('LogText');
	var objPeerList = document.getElementById('PeerList');
	var objUrl = document.getElementById('UrlInput');
	var objProtocol = document.getElementById('ProtocolInput');
	var objName = document.getElementById('NameInput');
	var objTarget = document.getElementById('TargetInput');
	var objMessage = document.getElementById('MessageInput');

	var procSetStatus = function(sText)
	{
		objStatus.textContent = '状态：' + sText;
	};

	var procSetSelf = function(sText)
	{
		objSelf.textContent = '当前 Peer：' + (sText || '-');
	};

	var procAppendLog = function(sText)
	{
		objLog.textContent += sText + '\n';
		objLog.scrollTop = objLog.scrollHeight;
	};

	var procRenderPeers = function()
	{
		var sHtml = '';

		if ( !arrPeers || arrPeers.length === 0 ) {
			objPeerList.innerHTML = '<div class="peer-empty">当前还没有在线 Peer。</div>';
			return;
		}

		arrPeers.forEach(function(tblPeer){
			var arrClass = ['peer-card'];

			if ( tblPeer.peer_id === sSelfID ) {
				arrClass.push('self');
			}
			sHtml += ''
				+ '<div class="' + arrClass.join(' ') + '" data-peer-id="' + tblPeer.peer_id + '">'
				+ '<strong>' + (tblPeer.name || tblPeer.peer_id) + '</strong>'
				+ '<span>' + tblPeer.peer_id + '</span>'
				+ '</div>';
		});

		objPeerList.innerHTML = sHtml;
		Array.prototype.slice.call(objPeerList.querySelectorAll('[data-peer-id]')).forEach(function(objNode){
			objNode.addEventListener('click', function(){
				objTarget.value = objNode.getAttribute('data-peer-id') || '';
			});
		});
	};

	var procCloseSocket = function()
	{
		if ( hSocket ) {
			try {
				hSocket.close();
			} catch (objErr) {
				procAppendLog('[close error] ' + String(objErr));
			}
			hSocket = null;
		}
	};

	var procSend = function(tblData)
	{
		if ( !hSocket || hSocket.readyState !== WebSocket.OPEN ) {
			procAppendLog('[send] websocket 未连接');
			return false;
		}
		hSocket.send(JSON.stringify(tblData));
		return true;
	};

	var procHandleMessage = function(tblData)
	{
		if ( !tblData || !tblData.kind ) {
			return;
		}

		if ( tblData.kind === 'welcome' ) {
			sSelfID = tblData.peer_id || '';
			arrPeers = tblData.peers || [];
			procSetSelf((tblData.name || tblData.peer_id || '-') + ' / ' + (tblData.peer_id || '-'));
			procRenderPeers();
			procAppendLog('[welcome] self=' + (tblData.peer_id || ''));
			return;
		}
		if ( tblData.kind === 'peers' ) {
			arrPeers = tblData.peers || [];
			procRenderPeers();
			procAppendLog('[peers] online=' + arrPeers.length);
			return;
		}
		if ( tblData.kind === 'direct' ) {
			procAppendLog('[' + (tblData.time || '-') + '] ' + (tblData.from_name || tblData.from) + ' -> ' + (tblData.to_name || tblData.to) + ' : ' + (tblData.text || ''));
			return;
		}
		if ( tblData.kind === 'error' ) {
			procAppendLog('[error] ' + (tblData.message || 'unknown'));
		}
	};

	fetch('/api/config')
	.then(function(objResp){
		return objResp.json();
	})
	.then(function(tblConfig){
		if ( tblConfig.ws_url ) {
			objUrl.value = tblConfig.ws_url;
		}
		if ( tblConfig.ws_protocol ) {
			objProtocol.value = tblConfig.ws_protocol;
		}
	})
	.catch(function(objErr){
		procAppendLog('[config error] ' + String(objErr));
	});

	document.getElementById('ConnectBtn').addEventListener('click', function(){
		procCloseSocket();
		procSetStatus('连接中');
		hSocket = objProtocol.value ? new WebSocket(objUrl.value, objProtocol.value) : new WebSocket(objUrl.value);
		hSocket.onopen = function()
		{
			procSetStatus('已连接');
			procAppendLog('[open] ' + objUrl.value);
			if ( objName.value.trim() ) {
				procSend({
					kind: 'hello',
					name: objName.value.trim()
				});
			}
		};
		hSocket.onmessage = function(objEvent)
		{
			var tblData = null;

			try {
				tblData = JSON.parse(objEvent.data);
			} catch (objErr) {
				procAppendLog('[recv raw] ' + objEvent.data);
				return;
			}
			procHandleMessage(tblData);
		};
		hSocket.onerror = function()
		{
			procSetStatus('连接错误');
			procAppendLog('[error] websocket error');
		};
		hSocket.onclose = function(objEvent)
		{
			procSetStatus('已断开');
			procAppendLog('[close] code=' + objEvent.code + ' reason=' + (objEvent.reason || ''));
			hSocket = null;
		};
	});

	document.getElementById('RenameBtn').addEventListener('click', function(){
		var sName = objName.value.trim();

		if ( !sName ) {
			procAppendLog('[rename] 昵称不能为空');
			return;
		}
		if ( procSend({ kind: 'hello', name: sName }) ) {
			procAppendLog('[rename] ' + sName);
		}
	});

	document.getElementById('SendBtn').addEventListener('click', function(){
		var sTarget = objTarget.value.trim();
		var sText = objMessage.value.trim();

		if ( !sTarget ) {
			procAppendLog('[send] 目标 peer_id 不能为空');
			return;
		}
		if ( !sText ) {
			procAppendLog('[send] 消息不能为空');
			return;
		}
		if ( procSend({ kind: 'direct', to: sTarget, text: sText }) ) {
			procAppendLog('[send] -> ' + sTarget + ' : ' + sText);
		}
	});

	document.getElementById('CloseBtn').addEventListener('click', function(){
		procCloseSocket();
	});

	document.getElementById('ClearBtn').addEventListener('click', function(){
		objLog.textContent = 'ready\n';
	});
})();
