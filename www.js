var groups = [];
var elements = [];
var logs = [];

var bleSvg = `<svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 -960 960 960" width="24"><path d="M440-80v-304L256-200l-56-56 224-224-224-224 56-56 184 184v-304h40l228 228-172 172 172 172L480-80h-40Zm80-496 76-76-76-74v150Zm0 342 76-74-76-76v150Z"/></svg>`;
var fingeprintSvg = `<svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 -960 960 960" width="24"><path d="M130-574q-7-5-8.5-12.5T126-602q62-85 155.5-132T481-781q106 0 200 45.5T838-604q7 9 4.5 16t-8.5 12q-6 5-14 4.5t-14-8.5q-55-78-141.5-119.5T481-741q-97 0-182 41.5T158-580q-6 9-14 10t-14-4ZM594-81q-104-26-170-103.5T358-374q0-50 36-84t87-34q51 0 87 34t36 84q0 33 25 55.5t59 22.5q34 0 58-22.5t24-55.5q0-116-85-195t-203-79q-118 0-203 79t-85 194q0 24 4.5 60t21.5 84q3 9-.5 16T208-205q-8 3-15.5-.5T182-217q-15-39-21.5-77.5T154-374q0-133 96.5-223T481-687q135 0 232 90t97 223q0 50-35.5 83.5T688-257q-51 0-87.5-33.5T564-374q0-33-24.5-55.5T481-452q-34 0-58.5 22.5T398-374q0 97 57.5 162T604-121q9 3 12 10t1 15q-2 7-8 12t-15 3ZM260-783q-8 5-16 2.5T232-791q-4-8-2-14.5t10-11.5q56-30 117-46t124-16q64 0 125 15.5T724-819q9 5 10.5 12t-1.5 14q-3 7-10 11t-17-1q-53-27-109.5-41.5T481-839q-58 0-114 13.5T260-783ZM378-95q-59-62-90.5-126.5T256-374q0-91 66-153.5T481-590q93 0 160 62.5T708-374q0 9-5.5 14.5T688-354q-8 0-14-5.5t-6-14.5q0-75-55.5-125.5T481-550q-76 0-130.5 50.5T296-374q0 81 28 137.5T406-123q6 6 6 14t-6 14q-6 6-14 6t-14-6Zm302-68q-89 0-154.5-60T460-374q0-8 5.5-14t14.5-6q9 0 14.5 6t5.5 14q0 75 54 123t126 48q6 0 17-1t23-3q9-2 15.5 2.5T744-191q2 8-3 14t-13 8q-18 5-31.5 5.5t-16.5.5Z"/></svg>`
var saveSvg = `<svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 -960 960 960" width="24"><path d="M840-680v480q0 33-23.5 56.5T760-120H200q-33 0-56.5-23.5T120-200v-560q0-33 23.5-56.5T200-840h480l160 160Zm-80 34L646-760H200v560h560v-446ZM480-240q50 0 85-35t35-85q0-50-35-85t-85-35q-50 0-85 35t-35 85q0 50 35 85t85 35ZM240-560h360v-160H240v160Zm-40-86v446-560 114Z"/></svg>`

document.head.insertAdjacentHTML('afterbegin', '<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@mdi/font@7.3.67/css/materialdesignicons.min.css">');
document.head.insertAdjacentHTML('afterbegin', '<meta name="viewport" content="viewport-fit=cover, width=device-width, initial-scale=1.0, minimum-scale=1.0, maximum-scale=1.0, user-scalable=no">');

var link = location.href;
const evtSource = new EventSource(`${link}events`);
evtSource.addEventListener('state' , (ev) => {
    const data = JSON.parse(ev.data);
    const id = '' + data.id;
    var index = elements.findIndex((v) => v.id === data.id);
    if (index > -1) {
        elements[index] = {...elements[index], ...data};
    } else {
        elements.push(data);
    }
    elements = elements.sort((a, b) => a.id > b.id ? 1 : -1);
    groups = elements.reduce((prev, curr) => {
        var foundGroup = prev.find((v) => v[0]?.name?.match(/_(.*?)_/)?.[1] === curr.name?.match(/_(.*?)_/)?.[1]);
        if (foundGroup) {
            foundGroup.push(curr);
        } else {
            prev.push([curr]);
        }
        return prev;
    }, []);

    if (index === -1) {
        renderGroups();
    } else {
        updateValues();
    }
   
});

function renderGroups() {
    for (var group of groups) {
        const groupId = group[0]?.name?.match(/_(.*?)_/)?.[1] || 'Other';
        let groupEl = document.getElementById(groupId);
        if (!groupEl) {
            document.getElementsByTagName('esp-app')[0].insertAdjacentHTML(groupId === 'Other' ? 'beforeend' : 'afterbegin', `<div id="${groupId}" class="group"><div class="header"><div>${groupId}</div><div class="buttons"></div></div></div>`);
        }
        groupEl = document.getElementById(groupId);
        for (var element of group) {
            removeElement(element.id);
            var icon = element.icon?.replace('mdi:', '');
            var name = element.name?.replace(element.name?.match(/_(.*?)_/)?.[0] || '', '');
    
            if (element.id?.startsWith('lock-')) {
                removeElement(element.id + '-close');
                removeElement(element.id + '-unlock');
                removeElement(element.id + '-open');
                
                groupEl.firstChild.lastChild.insertAdjacentHTML('beforeend', `<button onclick="buttonPress('${element.id}', 'lock')" id="${element.id}-close")"><span class="mdi mdi-lock-outline"></span></button>`);
                groupEl.firstChild.lastChild.insertAdjacentHTML('beforeend', `<button onclick="buttonPress('${element.id}', 'unlock')" id="${element.id}-unlock"><span class="mdi mdi-lock-open-outline"></span></button>`);
                groupEl.firstChild.lastChild.insertAdjacentHTML('beforeend', `<button onclick="buttonPress('${element.id}', 'open')" id="${element.id}-open"><span class="mdi mdi-lock-open-variant-outline"></span></button>`);
            }
            if (element.id?.startsWith('button-')) {
                groupEl.firstChild.lastChild.insertAdjacentHTML('beforeend', `<button onclick="buttonPress('${element.id}', 'press')" title="${name}" id="${element.id}"><span class="icon mdi mdi-${icon || 'content-save'}"></span></button>`);
            } else {
                var type = 'text';
                var readonly = !(element.id.startsWith('text-') || element.id.startsWith('number-') || element.id.startsWith('select-'));
                if (!readonly) {
                    type = element.id.startsWith('text-') ? 'text' :  element.id.startsWith('number-') ? 'number' : 'text';
                }
                var input = `<input class="inputElement" ${type == 'number' ? 'pattern="[0-9]*" inputmode="numeric"' : ''} min="${element.min_value}" max="${element.max_value}" ${readonly ? 'readonly' : ''} type="${type}" value="${ (readonly ? element.state : (element.value + '')) || ''}" onblur="setValue('${element.id}')"/>`;
                if (element.option) {
                    input = `<select class="inputElement" onchange="setValue('${element.id}')">${element.option.map((o) => `<option value="${o}" ${element.value == o ? 'selected="selected"' : ''} >${o}</option>`)}</select>`;
                }
                groupEl.insertAdjacentHTML('beforeend', `<div id="${element.id}" class="element"><div><span class="mdi mdi-${icon || 'information-slab-circle-outline'}"></span><span class="label">${name}</span></div>${input}</div>`);
            }
        }
    }
}
function updateValues() {
    for (const element of elements) {
        const el = document.getElementById(element.id);
        var inputElement = el.querySelector('.inputElement');
        if (inputElement) {
            var readonly = !(element.id.startsWith('text-') || element.id.startsWith('number-') || element.id.startsWith('select-'));
            el.querySelector('.inputElement').value = readonly ? element.state : (element.value + '');
        }
        var labelElement = el.querySelector('.label');
        if (labelElement) {
            var name = element.name?.replace(element.name?.match(/_(.*?)_/)?.[0] || '', '');
            labelElement.innerHTML = name;
        }
        var iconElement = el.querySelector('.icon');
        if (iconElement) {
            var icon = element.icon?.replace('mdi:', '');
            iconElement.classList = `icon mdi mdi-${icon || 'content-save'}`;
        }
    }
}
evtSource.addEventListener('ping' , (ev) => {
    removeElement('header');
    const data = JSON.parse(ev.data);
    document.title = data.title;
    document.getElementsByTagName('esp-app')[0].insertAdjacentHTML('beforebegin', `<div id="header">${bleSvg}${data.title}${fingeprintSvg}</div>`);
    if (data.log) {
        removeElement('log');       
        document.getElementsByTagName('esp-app')[0].insertAdjacentHTML('beforeend', '<div id="log"><div>Logs</div><table><thead><tr><th>Time</th><th>level</th><th>Tag</th><th>Message</th></tr></thead><tbody></tbody></table></div>');
    }
})
evtSource.addEventListener('log' , (ev) => {
    console.log(ev.data);
    const i = ev.data;
    let n = i.slice(10, i.length - 4).split(":").slice(0, 2).join(":")
    let o = i.slice(12 + n.length, i.length - 4);
    const l = {
        type: {
            "[1;31m": "e",
            "[0;33m": "w",
            "[0;32m": "i",
            "[0;35m": "c",
            "[0;36m": "d",
            "[0;37m": "v"
        }[i.slice(0, 7)],
        level: i.slice(7, 10),
        tag: n,
        detail: o,
        when: new Date().toTimeString().split(" ")[0]
    };
    logs.push(l);
    logs = logs.slice(-100);
    document.getElementById('log')?.querySelector('tbody')?.parentElement?.removeChild(document.getElementById('log')?.querySelector('tbody'));
    document.getElementById('log')?.querySelector('table')?.insertAdjacentHTML('beforeend', `<tbody>${logs.map(r=>`<tr class="${r.type}"><td>${r.when}</td><td>${r.level}</td><td>${r.tag}</td><td><pre>${r.detail}</pre></td></tr>`).join('')}</tbody>`);
    
})

function removeElement(id) {
    const el = document.getElementById(id);
    if (el) {
        el.parentElement.removeChild(el);
    }
}

window.setValue = function setValue(id) {
    var type = id.split('-')[0];
    var el = id.split('-')[1];
    var value = document.getElementById(id).querySelector('.inputElement').value;
    var valueType = type === 'select' ? 'option': 'value';
    var currentValue =  elements.find((e) => e.id === id)?.value;
    if (value !== currentValue) {
        fetch(`${link}${type}/${el}/set?${valueType}=${encodeURIComponent(value)}`, {
            method: "POST",
            headers: {
                'Content-Type': 'text/plain;charset=UTF-8'
            },
            body: "true"
        });
    }

}

window.buttonPress = function buttonPress(id, action) {
    var type = id.split('-')[0];
    var el = id.split('-')[1];
    fetch(`${link}${type}/${el}/${encodeURIComponent(action)}`, {method: "POST"});
}
