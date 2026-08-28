(function(){
	var objPeerList = document.getElementById('PeerList');
	var objLogList = document.getElementById('LogList');

	var procRenderPeers = function(arrItems)
	{
		if ( !arrItems || arrItems.length === 0 ) {
			objPeerList.innerHTML = '<div class="empty">当前没有在线客户端。</div>';
			return;
		}

		objPeerList.innerHTML = arrItems.map(function(tblItem){
			return ''
				+ '<div class="card">'
				+ '<strong>' + (tblItem.name || tblItem.peer_id || '') + '</strong>'
				+ '<span>peer_id：' + (tblItem.peer_id || '') + '</span>'
				+ '<span>在线：' + (Number(tblItem.online || 0) ? '是' : '否') + '</span>'
				+ '<span>最近内容：' + (tblItem.last_text || '') + '</span>'
				+ '<span>更新时间：' + (tblItem.updated_at || '') + '</span>'
				+ '</div>';
		}).join('');
	};

	var procRenderLogs = function(arrItems)
	{
		if ( !arrItems || arrItems.length === 0 ) {
			objLogList.innerHTML = '<div class="empty">当前没有消息。</div>';
			return;
		}

		objLogList.innerHTML = arrItems.map(function(tblItem){
			return ''
				+ '<div class="card">'
				+ '<strong>' + (tblItem.name || tblItem.peer_id || '') + '</strong>'
				+ '<span>时间：' + (tblItem.created_at || '') + '</span>'
				+ '<span>peer_id：' + (tblItem.peer_id || '') + '</span>'
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

	var procLoadLogs = function()
	{
		fetch('/api/log/list')
		.then(function(objResp){
			return objResp.json();
		})
		.then(function(tblData){
			procRenderLogs(tblData.items || []);
		})
		.catch(function(objErr){
			objLogList.innerHTML = '<div class="empty">加载失败：' + String(objErr) + '</div>';
		});
	};

	document.getElementById('RefreshPeerBtn').addEventListener('click', procLoadPeers);
	document.getElementById('RefreshLogBtn').addEventListener('click', procLoadLogs);
	procLoadPeers();
	procLoadLogs();
	setInterval(procLoadPeers, 1500);
	setInterval(procLoadLogs, 1500);
})();
