(function(){
	var objPeerList = document.getElementById('PeerList');
	var objChatList = document.getElementById('ChatList');

	var procRenderPeers = function(arrItems)
	{
		if ( !arrItems || arrItems.length === 0 ) {
			objPeerList.innerHTML = '<div class="empty">当前没有在线 peer。</div>';
			return;
		}

		objPeerList.innerHTML = arrItems.map(function(tblItem){
			return ''
				+ '<div class="card">'
				+ '<strong>' + (tblItem.name || '') + '</strong>'
				+ '<span>远端：' + (tblItem.remote || '') + '</span>'
				+ '<span>最近内容：' + (tblItem.last_text || '') + '</span>'
				+ '<span>更新时间：' + (tblItem.updated_at || '') + '</span>'
				+ '</div>';
		}).join('');
	};

	var procRenderChats = function(arrItems)
	{
		if ( !arrItems || arrItems.length === 0 ) {
			objChatList.innerHTML = '<div class="empty">当前没有聊天记录。</div>';
			return;
		}

		objChatList.innerHTML = arrItems.map(function(tblItem){
			return ''
				+ '<div class="card">'
				+ '<strong>' + (tblItem.from_name || '') + ' -> ' + (tblItem.to_name || '(none)') + '</strong>'
				+ '<span>时间：' + (tblItem.created_at || '') + '</span>'
				+ '<span>远端：' + (tblItem.remote || '') + '</span>'
				+ '<span>内容：' + (tblItem.text || '') + '</span>'
				+ '</div>';
		}).join('');
	};

	var procLoadPeers = function()
	{
		fetch('/api/peer/list')
		.then(function(objResp){
			return objResp.json();
		})
		.then(function(tblData){
			procRenderPeers(tblData.items || []);
		})
		.catch(function(objErr){
			objPeerList.innerHTML = '<div class="empty">加载失败：' + String(objErr) + '</div>';
		});
	};

	var procLoadChats = function()
	{
		fetch('/api/chat/list')
		.then(function(objResp){
			return objResp.json();
		})
		.then(function(tblData){
			procRenderChats(tblData.items || []);
		})
		.catch(function(objErr){
			objChatList.innerHTML = '<div class="empty">加载失败：' + String(objErr) + '</div>';
		});
	};

	document.getElementById('RefreshPeerBtn').addEventListener('click', procLoadPeers);
	document.getElementById('RefreshChatBtn').addEventListener('click', procLoadChats);
	procLoadPeers();
	procLoadChats();
	setInterval(procLoadPeers, 1500);
	setInterval(procLoadChats, 1500);
})();
