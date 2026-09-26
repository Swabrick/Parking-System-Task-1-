// Saint Hotel Parking web client. Handles page updates, vehicle operations, payments and management controls.
let slots = [];
let lastReceipt = null;
let pendingPaymentId = null;
let selectedPaymentMethod = null;
const $ = id => document.getElementById(id);

const sectionTitles = {
    dashboard: 'Dashboard', parking: 'Parking Layout', vehicles: 'Vehicle Operations',
    live: 'Live Vehicles', records: 'Parking Records', analytics: 'Analytics', management: 'Management'
};

document.querySelectorAll('.nav-item').forEach(button => button.addEventListener('click', () => showSection(button.dataset.section)));

function showSection(id) {
    document.querySelectorAll('.section').forEach(s => s.classList.remove('active-section'));
    $(id).classList.add('active-section');
    document.querySelectorAll('.nav-item').forEach(b => b.classList.toggle('active', b.dataset.section === id));
    $('pageTitle').textContent = sectionTitles[id];
    if (id === 'parking') renderLayout();
    if (id === 'live') loadActive();
    if (id === 'records') loadRecords();
    if (id === 'analytics') loadAnalytics();
    if (id === 'management') loadManagement();
}

function openModal(id) { $(id).classList.add('open'); }
function closeModal(id) { $(id).classList.remove('open'); }

async function getJSON(url, options) {
    const response = await fetch(url, options);
    const data = await response.json();
    if (!response.ok) throw new Error(data.message || 'Request failed');
    return data;
}

function money(value) { return 'KSh ' + Number(value || 0).toLocaleString(); }
function escapeHtml(value) { return String(value).replace(/[&<>'"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;',"'":'&#39;','"':'&quot;'}[c])); }

async function refresh() {
    try {
        const [status, slotData, active, transactions, analytics, barriers] = await Promise.all([
            getJSON('/api/status'), getJSON('/api/slots'), getJSON('/api/active'),
            getJSON('/api/transactions'), getJSON('/api/analytics'), getJSON('/api/barriers')
        ]);
        slots = slotData;
        $('total').textContent = status.total;
        $('available').textContent = status.available;
        $('occupied').textContent = status.occupied;
        $('revenue').textContent = money(analytics.revenue);
        renderMiniFloors();
        renderLayout();
        renderBarriers(barriers);
        renderPricing(status);
        renderActivePreview(active);
        renderRecent(transactions);
        if ($('live').classList.contains('active-section')) renderActive(active);
        if ($('records').classList.contains('active-section')) renderRecords(transactions);
        if ($('analytics').classList.contains('active-section')) renderAnalytics(analytics);
        if ($('management').classList.contains('active-section')) loadManagement();
    } catch (e) {
        showToast('Could not connect to the local server.');
    }
}

function renderMiniFloors() {
    const container = $('miniFloors');
    container.innerHTML = '';
    for (let floor = 1; floor <= 5; floor++) {
        const row = document.createElement('div');
        row.className = 'mini-floor';
        row.innerHTML = `<span class="floor-label">Floor ${floor}</span><div class="slot-line"></div>`;
        const line = row.querySelector('.slot-line');
        slots.filter(s => s.floor === floor).forEach(s => {
            const el = document.createElement('span');
            el.className = 'slot ' + (s.status === 'occupied' ? 'occupied' : '');
            el.title = s.code + (s.vehicle ? ' · ' + s.vehicle : '');
            line.appendChild(el);
        });
        container.appendChild(row);
    }
}

function renderLayout() {
    const container = $('floorLayout');
    if (!container) return;
    container.innerHTML = '';
    for (let floor = 1; floor <= 5; floor++) {
        const card = document.createElement('article');
        card.className = 'floor-card';
        const floorSlots = slots.filter(s => s.floor === floor);
        const free = floorSlots.filter(s => s.status === 'available').length;
        card.innerHTML = `<div class="floor-card-head"><div><h3>Floor ${floor}</h3><span>${free} available</span></div></div><div class="wing-grid"></div>`;
        const grid = card.querySelector('.wing-grid');
        ['A', 'B'].forEach(wing => {
            const box = document.createElement('div');
            box.innerHTML = `<div class="wing-title">WING ${wing}</div><div class="slot-grid"></div>`;
            const sg = box.querySelector('.slot-grid');
            floorSlots.filter(s => s.wing === wing).forEach(s => {
                const el = document.createElement('button');
                el.className = 'full-slot ' + (s.status === 'occupied' ? 'occupied' : '');
                el.textContent = s.code;
                el.title = s.status === 'occupied' ? s.vehicle : 'Available';
                el.onclick = () => slotDetails(s);
                sg.appendChild(el);
            });
            grid.appendChild(box);
        });
        container.appendChild(card);
    }
}

function slotDetails(slot) {
    if (slot.status === 'available') {
        showToast(slot.code + ' is available.');
        return;
    }
    showToast(slot.code + ' is occupied by ' + slot.vehicle + '.');
}

function renderBarriers(b) {
    $('barrierSummary').innerHTML = `<div class="barrier-row"><span>Entrance</span><strong class="gate ${b.entrance === 'open' ? 'open' : ''}">${b.entrance.toUpperCase()}</strong></div><div class="barrier-row"><span>Exit</span><strong class="gate ${b.exit === 'open' ? 'open' : ''}">${b.exit.toUpperCase()}</strong></div><div class="barrier-row"><span>Mode</span><strong>${b.automatic ? 'AUTOMATIC' : 'MANUAL'}</strong></div>`;
    if ($('barrierControls')) {
        $('barrierControls').innerHTML = `<div class="control-row"><span>Entrance</span><button onclick="barrier('entrance','open')">Open</button><button onclick="barrier('entrance','close')">Close</button></div><div class="control-row"><span>Exit</span><button onclick="barrier('exit','open')">Open</button><button onclick="barrier('exit','close')">Close</button></div>`;
    }
}

function renderPricing(status) {
    $('pricingList').innerHTML = `<div><span>Up to ${status.rateFree} minutes</span><strong>Free</strong></div><div><span>Up to ${status.rate1Limit} minutes</span><strong>${money(status.fee1)}</strong></div><div><span>Up to ${status.rate2Limit} minutes</span><strong>${money(status.fee2)}</strong></div><div><span>Up to ${status.rate3Limit} minutes</span><strong>${money(status.fee3)}</strong></div><div><span>Over ${status.rate3Limit} minutes</span><strong>${money(status.feeMax)}</strong></div>`;
}

function renderActivePreview(active) {
    $('activePreview').innerHTML = active.length
        ? active.slice(0, 5).map(v => `<div class="compact-row"><strong>${escapeHtml(v.vehicle)}</strong><span>${v.slot}</span><span>${v.minutes} min</span></div>`).join('')
        : '<div class="empty">No vehicles currently inside.</div>';
}

function renderRecent(records) {
    $('recentPreview').innerHTML = records.length
        ? records.slice(0, 5).map(r => `<div class="compact-row"><strong>${escapeHtml(r.vehicle)}</strong><span>${r.slot}</span><span>${money(r.amount)}</span></div>`).join('')
        : '<div class="empty">No recorded exits.</div>';
}

async function submitEntry(event) {
    event.preventDefault();
    try {
        const vehicle = $('entryVehicle').value;
        const data = await getJSON('/api/entry', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body: 'vehicle=' + encodeURIComponent(vehicle)
        });
        $('entryMessage').textContent = data.message + ' Slot: ' + data.slot;
        $('entryMessage').className = 'form-message success';
        $('entryVehicle').value = '';
        showToast(data.barrier === 'open' ? 'Entrance barrier opened automatically.' : 'Open the entrance barrier manually.');
        await refresh();
        setTimeout(() => closeModal('entryModal'), 900);
    } catch (e) {
        $('entryMessage').textContent = e.message;
        $('entryMessage').className = 'form-message error';
    }
}

async function submitExit(event) {
    event.preventDefault();
    try {
        const vehicle = $('exitVehicle').value;
        const data = await getJSON('/api/exit', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body: 'vehicle=' + encodeURIComponent(vehicle)
        });

        lastReceipt = data;
        pendingPaymentId = data.amount > 0 ? data.transactionId : null;
        selectedPaymentMethod = null;
        $('selectedPayment').textContent = 'No payment method selected.';
        $('exitMessage').innerHTML = `${escapeHtml(data.message)}<br>Duration: <strong>${data.duration} minute(s)</strong><br>Amount: <strong>${money(data.amount)}</strong>`;
        $('exitMessage').className = 'form-message success';
        $('exitAmount').textContent = money(data.amount);
        $('paymentBox').classList.remove('hidden');
        $('confirmPaymentButton').disabled = data.amount === 0;
        $('exitVehicle').value = '';

        if (data.amount === 0) {
            $('paymentInstruction').textContent = 'No payment was required. The vehicle can leave and the receipt can be printed.';
            showToast(data.barrier === 'open' ? 'Exit barrier opened automatically.' : 'Open the exit barrier manually.');
        } else {
            $('paymentInstruction').textContent = 'Collect the payment from the driver. Cash is supported for manual collection; electronic payment integrations are coming soon.';
            showToast('Payment is required before the exit barrier opens.');
        }

        await refresh();
    } catch (e) {
        $('exitMessage').textContent = e.message;
        $('exitMessage').className = 'form-message error';
    }
}

function selectPaymentMethod(method) {
    if (method !== 'cash') return paymentComingSoon(method);
    selectedPaymentMethod = method;
    $('selectedPayment').textContent = 'Selected payment method: Cash';
    $('cashPaymentButton').classList.add('selected');
}

function paymentComingSoon(method) {
    showToast(method + ' payment integration: Coming soon.');
}

async function confirmPayment() {
    if (!pendingPaymentId || selectedPaymentMethod !== 'cash') {
        showToast('Select Cash after the operator has collected the payment.');
        return;
    }

    try {
        const body = `id=${encodeURIComponent(pendingPaymentId)}&paymentMethod=${encodeURIComponent(selectedPaymentMethod)}`;
        const data = await getJSON('/api/payment/confirm', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body
        });
        lastReceipt = data;
        pendingPaymentId = null;
        $('exitMessage').innerHTML = `${escapeHtml(data.message)}<br>Payment: <strong>${escapeHtml(data.paymentMethod)}</strong>`;
        $('exitMessage').className = 'form-message success';
        $('confirmPaymentButton').disabled = true;
        showToast(data.barrier === 'open' ? 'Payment confirmed. Exit barrier opened automatically.' : 'Payment confirmed. Open the exit barrier manually.');
        await refresh();
    } catch (e) {
        $('exitMessage').textContent = e.message;
        $('exitMessage').className = 'form-message error';
    }
}

function printReceipt() {
    if (!lastReceipt) return;
    const r = lastReceipt;
    const receipt = `<!doctype html><html><head><title>Parking Receipt</title><style>body{font-family:Arial,sans-serif;width:320px;margin:30px auto;color:#111}h1{font-size:20px;margin-bottom:4px}p{font-size:12px;color:#555}.line{border-top:1px solid #ddd;margin:18px 0}.row{display:flex;justify-content:space-between;margin:9px 0;font-size:13px}.total{font-size:17px;font-weight:700}.print{margin-top:25px;padding:10px;width:100%}@media print{.print{display:none}}</style></head><body><h1>SAINT HOTEL</h1><p>Parking System</p><div class="line"></div><div class="row"><span>Vehicle</span><strong>${escapeHtml(r.vehicle)}</strong></div><div class="row"><span>Space</span><strong>${escapeHtml(r.slot)}</strong></div><div class="row"><span>Entry</span><span>${escapeHtml(r.entry)}</span></div><div class="row"><span>Exit</span><span>${escapeHtml(r.exit)}</span></div><div class="row"><span>Duration</span><span>${r.duration} minute(s)</span></div><div class="line"></div><div class="row total"><span>Amount</span><span>${money(r.amount)}</span></div><p>Payment: ${escapeHtml(r.paymentMethod || (r.amount === 0 ? 'Free' : 'Pending'))}</p><p>Thank you for using the parking facility.</p><button class="print" onclick="window.print()">Print</button></body></html>`;
    const win = window.open('', '_blank', 'width=420,height=650');
    if (win) {
        win.document.write(receipt);
        win.document.close();
        setTimeout(() => win.print(), 300);
    }
}

function findVehicle() {
    const value = $('searchVehicle').value.trim().toUpperCase().replace(/\s+/g, '');
    const slot = slots.find(s => s.status === 'occupied' && s.vehicle === value);
    $('searchResult').innerHTML = slot
        ? `Vehicle <strong>${escapeHtml(slot.vehicle)}</strong> is parked at <strong>${slot.code}</strong> and entered at <strong>${escapeHtml(slot.entry)}</strong>.`
        : 'No active vehicle with that registration was found.';
}

async function loadActive() {
    const active = await getJSON('/api/active');
    renderActive(active);
}

function renderActive(active) {
    $('activeBody').innerHTML = active.length
        ? active.map(v => `<tr><td><strong>${escapeHtml(v.vehicle)}</strong></td><td>${v.slot}</td><td>Floor ${v.floor} · Wing ${v.wing}</td><td>${escapeHtml(v.entry)}</td><td>${v.minutes} min</td><td>${money(v.estimatedFee)}</td><td>${v.paymentPending ? '<span class="pending-label">Payment pending</span>' : 'Inside'}</td></tr>`).join('')
        : '<tr><td colspan="7">No vehicles are currently inside.</td></tr>';
}

async function loadRecords() {
    const records = await getJSON('/api/transactions');
    renderRecords(records);
}

function renderRecords(records) {
    $('recordsBody').innerHTML = records.length
        ? records.map(r => `<tr><td>${r.id}</td><td><strong>${escapeHtml(r.vehicle)}</strong></td><td>${r.slot}</td><td>${escapeHtml(r.entry)}</td><td>${escapeHtml(r.exit)}</td><td>${r.duration} min</td><td>${money(r.amount)}</td><td>${escapeHtml(r.paymentStatus)}<br><small>${escapeHtml(r.paymentMethod)}</small></td><td><button class="table-action" onclick='editTransaction(${JSON.stringify(r)})'>Edit</button></td></tr>`).join('')
        : '<tr><td colspan="9">No parking records yet.</td></tr>';
}

function editTransaction(r) {
    const form = $('editForm');
    form.id.value = r.id;
    form.amount.value = r.amount;
    form.paymentStatus.value = r.paymentStatus.toLowerCase();
    form.paymentMethod.value = r.paymentMethod === 'M-Pesa' ? 'mpesa' : (r.paymentMethod === 'Not selected' ? 'none' : r.paymentMethod.toLowerCase());
    openModal('editModal');
}

$('editForm').addEventListener('submit', async event => {
    event.preventDefault();
    const form = new FormData(event.target);
    try {
        const body = new URLSearchParams(form).toString();
        const data = await getJSON('/api/transactions/update', {
            method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body
        });
        $('editMessage').textContent = data.message;
        $('editMessage').className = 'form-message success';
        await refresh();
        setTimeout(() => closeModal('editModal'), 700);
    } catch (e) {
        $('editMessage').textContent = e.message;
        $('editMessage').className = 'form-message error';
    }
});

async function loadAnalytics() { renderAnalytics(await getJSON('/api/analytics')); }

function renderAnalytics(a) {
    $('analyticsRevenue').textContent = money(a.revenue);
    $('averageStay').textContent = Math.round(a.averageMinutes) + ' min';
    $('paidVisits').textContent = a.paid;
    $('pendingPayments').textContent = a.pending;
    const max = Math.max(...a.floorUse, 1);
    $('floorAnalytics').innerHTML = a.floorUse.map((v, i) => `<div class="bar-row"><span>Floor ${i + 1}</span><div><i style="width:${(v / max) * 100}%"></i></div><strong>${v}</strong></div>`).join('');
    const total = Math.max(a.paid + a.free, 1);
    $('sessionMix').innerHTML = `<div class="mix-total"><strong>${a.paid + a.free}</strong><span>completed sessions</span></div><div class="mix-row"><span>Paid</span><div><i style="width:${(a.paid / total) * 100}%"></i></div><strong>${a.paid}</strong></div><div class="mix-row"><span>Free</span><div><i style="width:${(a.free / total) * 100}%"></i></div><strong>${a.free}</strong></div><div class="mix-row"><span>Pending</span><div><i style="width:${(a.pending / Math.max(a.pending + total, 1)) * 100}%"></i></div><strong>${a.pending}</strong></div>`;
}

async function loadManagement() {
    const [settings, blacklist, barriers] = await Promise.all([
        getJSON('/api/settings'), getJSON('/api/blacklist'), getJSON('/api/barriers')
    ]);
    const form = $('pricingForm');
    Object.keys(settings).forEach(k => { if (form[k]) form[k].value = settings[k]; });
    $('blacklistItems').innerHTML = blacklist.length
        ? blacklist.map(v => `<span class="tag">${escapeHtml(v)} <button onclick="removeBlacklist('${escapeHtml(v)}')">×</button></span>`).join('')
        : '<div class="empty">No blacklisted vehicles.</div>';
    renderBarriers(barriers);
}

$('pricingForm').addEventListener('submit', async event => {
    event.preventDefault();
    try {
        const body = new URLSearchParams(new FormData(event.target)).toString();
        const data = await getJSON('/api/settings/pricing', {
            method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body
        });
        $('pricingMessage').textContent = data.message;
        $('pricingMessage').className = 'form-message success';
        await refresh();
    } catch (e) {
        $('pricingMessage').textContent = e.message;
        $('pricingMessage').className = 'form-message error';
    }
});

$('blacklistForm').addEventListener('submit', async event => {
    event.preventDefault();
    try {
        const v = $('blacklistVehicle').value;
        const data = await getJSON('/api/blacklist/add', {
            method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'vehicle=' + encodeURIComponent(v)
        });
        showToast(data.message);
        $('blacklistVehicle').value = '';
        loadManagement();
    } catch (e) { showToast(e.message); }
});

async function removeBlacklist(v) {
    try {
        const data = await getJSON('/api/blacklist/remove', {
            method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: 'vehicle=' + encodeURIComponent(v)
        });
        showToast(data.message);
        loadManagement();
    } catch (e) { showToast(e.message); }
}

async function barrier(name, action) {
    try {
        const data = await getJSON('/api/barrier', {
            method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `barrier=${name}&action=${action}`
        });
        showToast(data.message);
        refresh();
    } catch (e) { showToast(e.message); }
}

async function setBarrierMode(mode) {
    try {
        const data = await getJSON('/api/barrier', {
            method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'}, body: `barrier=mode&action=${mode}`
        });
        showToast(data.message);
        refresh();
    } catch (e) { showToast(e.message); }
}

function showToast(text) {
    const toast = $('toast');
    toast.textContent = text;
    toast.classList.add('show');
    setTimeout(() => toast.classList.remove('show'), 2600);
}

function updateClock() {
    $('clock').textContent = new Date().toLocaleTimeString([], {hour: '2-digit', minute: '2-digit', second: '2-digit'});
}

setInterval(updateClock, 1000);
updateClock();
refresh();
setInterval(refresh, 5000);
