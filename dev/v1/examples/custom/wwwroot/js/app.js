(function(){
	var objTaskList = document.getElementById('TaskList');
	var objRunList = document.getElementById('RunList');

	var procNormTime = function(sValue)
	{
		if ( !sValue ) {
			return '';
		}
		if ( sValue.length === 16 ) {
			return sValue.replace('T', ' ') + ':00';
		}
		return sValue.replace('T', ' ');
	};

	var procPostJSON = function(sUrl, tblData)
	{
		return fetch(sUrl, {
			method: 'POST',
			headers: {
				'Content-Type': 'application/json'
			},
			body: JSON.stringify(tblData)
		}).then(function(objResp){
			return objResp.json();
		});
	};

	var procRenderTasks = function(arrItems)
	{
		if ( !arrItems || arrItems.length === 0 ) {
			objTaskList.innerHTML = '<div class="empty">当前没有任务。</div>';
			return;
		}

		objTaskList.innerHTML = arrItems.map(function(tblItem){
			return ''
				+ '<div class="card">'
				+ '<strong>' + (tblItem.name || '') + '</strong>'
				+ '<span>执行时间：' + (tblItem.run_at || '') + '</span>'
				+ '<span>命令：' + (tblItem.command || '') + '</span>'
				+ '<span>工作目录：' + (tblItem.workdir || '(默认)') + '</span>'
				+ '<span>启用：' + (Number(tblItem.enabled || 0) ? '是' : '否') + '</span>'
				+ '<span>上次执行：' + (tblItem.last_run_at || '-') + '</span>'
				+ '<span>上次状态：' + (tblItem.last_status || '-') + '</span>'
				+ '<div class="card-actions">'
				+ '<button class="btn alt" data-action="toggle" data-id="' + (tblItem.id || '') + '" data-enabled="' + (Number(tblItem.enabled || 0) ? '0' : '1') + '">' + (Number(tblItem.enabled || 0) ? '停用' : '启用') + '</button>'
				+ '<button class="btn warn" data-action="del" data-id="' + (tblItem.id || '') + '">删除</button>'
				+ '</div>'
				+ '</div>';
		}).join('');

		Array.prototype.slice.call(objTaskList.querySelectorAll('[data-action="toggle"]')).forEach(function(objBtn){
			objBtn.addEventListener('click', function(){
				procPostJSON('/api/task/toggle', {
					id: Number(objBtn.getAttribute('data-id') || '0'),
					enabled: Number(objBtn.getAttribute('data-enabled') || '0')
				}).then(function(){
					procLoadTasks();
				});
			});
		});

		Array.prototype.slice.call(objTaskList.querySelectorAll('[data-action="del"]')).forEach(function(objBtn){
			objBtn.addEventListener('click', function(){
				procPostJSON('/api/task/del', {
					id: Number(objBtn.getAttribute('data-id') || '0')
				}).then(function(){
					procLoadTasks();
					procLoadRuns();
				});
			});
		});
	};

	var procRenderRuns = function(arrItems)
	{
		if ( !arrItems || arrItems.length === 0 ) {
			objRunList.innerHTML = '<div class="empty">当前没有执行记录。</div>';
			return;
		}

		objRunList.innerHTML = arrItems.map(function(tblItem){
			return ''
				+ '<div class="card">'
				+ '<strong>' + (tblItem.task_name || '') + '</strong>'
				+ '<span>时间：' + (tblItem.created_at || '') + '</span>'
				+ '<span>状态：' + (tblItem.status || '') + '</span>'
				+ '<span>退出码：' + (tblItem.exit_code || '') + '</span>'
				+ '<span>输出：' + (tblItem.output || '') + '</span>'
				+ '</div>';
		}).join('');
	};

	var procLoadTasks = function()
	{
		fetch('/api/task/list')
		.then(function(objResp){
			return objResp.json();
		})
		.then(function(tblData){
			procRenderTasks(tblData.items || []);
		})
		.catch(function(objErr){
			objTaskList.innerHTML = '<div class="empty">加载失败：' + String(objErr) + '</div>';
		});
	};

	var procLoadRuns = function()
	{
		fetch('/api/run/list')
		.then(function(objResp){
			return objResp.json();
		})
		.then(function(tblData){
			procRenderRuns(tblData.items || []);
		})
		.catch(function(objErr){
			objRunList.innerHTML = '<div class="empty">加载失败：' + String(objErr) + '</div>';
		});
	};

	document.getElementById('AddBtn').addEventListener('click', function(){
		var sName = document.getElementById('NameInput').value.trim();
		var sRunAt = procNormTime(document.getElementById('TimeInput').value.trim());
		var sCommand = document.getElementById('CommandInput').value.trim();
		var sWorkdir = document.getElementById('WorkdirInput').value.trim();

		if ( !sName || !sRunAt || !sCommand ) {
			alert('请填写 name / run_at / command');
			return;
		}

		procPostJSON('/api/task/add', {
			name: sName,
			run_at: sRunAt,
			command: sCommand,
			workdir: sWorkdir
		}).then(function(){
			procLoadTasks();
		});
	});

	document.getElementById('RefreshTaskBtn').addEventListener('click', procLoadTasks);
	document.getElementById('RefreshRunBtn').addEventListener('click', procLoadRuns);
	procLoadTasks();
	procLoadRuns();
	setInterval(procLoadTasks, 2000);
	setInterval(procLoadRuns, 2000);
})();
