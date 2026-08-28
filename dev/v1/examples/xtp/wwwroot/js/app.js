(function(){
	var objBody = document.getElementById('LogBody');

	var procRender = function(arrItems)
	{
		var sHtml = '';

		if ( !arrItems || arrItems.length === 0 ) {
			objBody.innerHTML = '<tr><td colspan="7"><div class="empty">当前还没有日志。</div></td></tr>';
			return;
		}

		arrItems.forEach(function(tblItem){
			sHtml += '<tr>'
				+ '<td>' + (tblItem.id || '') + '</td>'
				+ '<td>' + (tblItem.created_at || '') + '</td>'
				+ '<td>' + (tblItem.level || '') + '</td>'
				+ '<td>' + (tblItem.source || '') + '</td>'
				+ '<td>' + (tblItem.cmd || '') + '</td>'
				+ '<td>' + (tblItem.msg_type || '') + '</td>'
				+ '<td>' + (tblItem.message || '') + '</td>'
				+ '</tr>';
		});
		objBody.innerHTML = sHtml;
	};

	var procLoad = function()
	{
		fetch('/api/log/list')
		.then(function(objResp){
			return objResp.json();
		})
		.then(function(tblData){
			procRender(tblData.items || []);
		})
		.catch(function(objErr){
			objBody.innerHTML = '<tr><td colspan="7"><div class="empty">加载失败：' + String(objErr) + '</div></td></tr>';
		});
	};

	document.getElementById('RefreshBtn').addEventListener('click', procLoad);
	procLoad();
	setInterval(procLoad, 1500);
})();
