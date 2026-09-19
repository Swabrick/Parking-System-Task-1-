let slots=[];
const $=id=>document.getElementById(id);

function showSection(id){
    document.querySelectorAll('.section').forEach(s=>s.classList.remove('active-section'));
    $(id).classList.add('active-section');
    document.querySelectorAll('.nav-item').forEach(b=>b.classList.toggle('active',b.dataset.section===id));
    const titles={dashboard:'Dashboard',parking:'Parking Layout',vehicles:'Vehicle Operations',records:'Parking Records'};
    $('pageTitle').textContent=titles[id];
    if(id==='parking') renderLayout();
    if(id==='records') loadRecords();
}

document.querySelectorAll('.nav-item').forEach(button=>button.addEventListener('click',()=>showSection(button.dataset.section)));

function openModal(id){$(id).classList.add('open');}
function closeModal(id){$(id).classList.remove('open');}

async function getJSON(url,options){
    const response=await fetch(url,options);
    return response.json();
}

async function refresh(){
    try{
        const [status,slotData]=await Promise.all([getJSON('/api/status'),getJSON('/api/slots')]);
        slots=slotData;
        $('total').textContent=status.total;
        $('available').textContent=status.available;
        $('occupied').textContent=status.occupied;
        $('transactions').textContent=status.transactions;
        renderMiniFloors();
        renderLayout();
    }catch(e){showToast('Could not connect to the local server.');}
}

function renderMiniFloors(){
    const container=$('miniFloors');
    container.innerHTML='';
    for(let floor=1;floor<=5;floor++){
        const row=document.createElement('div'); row.className='mini-floor';
        row.innerHTML=`<span class="floor-label">Floor ${floor}</span><div class="slot-line"></div>`;
        const line=row.querySelector('.slot-line');
        slots.filter(s=>s.floor===floor).forEach(s=>{const el=document.createElement('span');el.className='slot '+(s.status==='occupied'?'occupied':'');el.title=s.code+(s.vehicle?' · '+s.vehicle:'');line.appendChild(el);});
        container.appendChild(row);
    }
}

function renderLayout(){
    const container=$('floorLayout');
    if(!container)return;
    container.innerHTML='';
    for(let floor=1;floor<=5;floor++){
        const card=document.createElement('article'); card.className='floor-card';
        const floorSlots=slots.filter(s=>s.floor===floor);
        const free=floorSlots.filter(s=>s.status==='available').length;
        card.innerHTML=`<div class="floor-card-head"><h3>Floor ${floor}</h3><span class="eyebrow">${free} AVAILABLE</span></div><div class="wing-grid"></div>`;
        const grid=card.querySelector('.wing-grid');
        ['A','B'].forEach(wing=>{
            const box=document.createElement('div');
            box.innerHTML=`<div class="wing-title">WING ${wing}</div><div class="slot-grid"></div>`;
            const sg=box.querySelector('.slot-grid');
            floorSlots.filter(s=>s.wing===wing).forEach(s=>{
                const el=document.createElement('div'); el.className='full-slot '+(s.status==='occupied'?'occupied':''); el.textContent=s.code;
                el.title=s.status==='occupied'?s.vehicle:'Available'; sg.appendChild(el);
            });
            grid.appendChild(box);
        });
        container.appendChild(card);
    }
}

async function submitEntry(event){
    event.preventDefault();
    const vehicle=$('entryVehicle').value;
    const data=await getJSON('/api/entry',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'vehicle='+encodeURIComponent(vehicle)});
    $('entryMessage').textContent=data.message+(data.slot?' Slot: '+data.slot:'');
    $('entryMessage').style.color=data.success?'#25633a':'#b11';
    if(data.success){$('entryVehicle').value='';showToast(data.message);await refresh();setTimeout(()=>closeModal('entryModal'),900);}
}

async function submitExit(event){
    event.preventDefault();
    const vehicle=$('exitVehicle').value;
    const data=await getJSON('/api/exit',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'vehicle='+encodeURIComponent(vehicle)});
    $('exitMessage').innerHTML=data.success?`${data.message}<br>Duration: <strong>${data.duration} minute(s)</strong><br>Amount: <strong>KSh ${data.amount}</strong>`:data.message;
    $('exitMessage').style.color=data.success?'#25633a':'#b11';
    if(data.success){$('exitVehicle').value='';showToast('Exit recorded successfully.');await refresh();setTimeout(()=>closeModal('exitModal'),1400);}
}

function findVehicle(){
    const value=$('searchVehicle').value.trim().toUpperCase().replace(/\s+/g,'');
    const slot=slots.find(s=>s.status==='occupied'&&s.vehicle===value);
    $('searchResult').innerHTML=slot?`Vehicle <strong>${slot.vehicle}</strong> is parked at <strong>${slot.code}</strong>.`:'No active vehicle with that registration was found.';
}

async function loadRecords(){
    const records=await getJSON('/api/transactions');
    const body=$('recordsBody'); body.innerHTML='';
    if(!records.length){body.innerHTML='<tr><td colspan="6">No completed parking sessions yet.</td></tr>';return;}
    records.forEach(r=>{const row=document.createElement('tr');row.innerHTML=`<td><strong>${r.vehicle}</strong></td><td>${r.slot}</td><td>${r.entry}</td><td>${r.exit}</td><td>${r.duration} min</td><td>KSh ${r.amount}</td>`;body.appendChild(row);});
}

function showToast(text){const toast=$('toast');toast.textContent=text;toast.classList.add('show');setTimeout(()=>toast.classList.remove('show'),2200);}
function updateClock(){$('clock').textContent=new Date().toLocaleTimeString([], {hour:'2-digit',minute:'2-digit',second:'2-digit'});}
setInterval(updateClock,1000);updateClock();refresh();
