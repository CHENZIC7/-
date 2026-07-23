// 校园微农园智能监测系统 - 主要JavaScript文件

// 登录验证
function checkAuthentication() {
    const isLoggedIn = localStorage.getItem('isLoggedIn');
    if (isLoggedIn !== 'true') {
        window.location.href = 'login.html';
        return false;
    }

    // 显示当前用户信息
    const currentUser = JSON.parse(localStorage.getItem('currentUser') || '{}');
    const userNameElement = document.getElementById('userName');
    if (userNameElement && currentUser.name) {
        userNameElement.textContent = currentUser.name;
    }
    return true;
}

// 退出登录
function logout() {
    if (confirm('确定要退出登录吗？')) {
        localStorage.removeItem('isLoggedIn');
        localStorage.removeItem('currentUser');
        window.location.href = 'login.html';
    }
}

// 页面加载时检查登录状态
if (!checkAuthentication()) {
    // 如果未登录，阻止后续代码执行
    throw new Error('未登录');
}

// 全局变量
let sensorData = {
    temperature: 25,
    humidity: 55,
    soilMoisture: 65,
    lightIntensity: 3200,
    soilPH: 6.5,
    co2Concentration: 650
};

// 图表实例
let tempHumidityChart, soilLightChart, co2Chart, phChart;

// 设备状态
const deviceStatus = {
    irrigation: false,
    shade: false,
    ventilation: true,
    light: false,
    ai: true,
    notification: true
};

// 初始化函数
document.addEventListener('DOMContentLoaded', function() {
    // 更新时间
    updateTime();
    setInterval(updateTime, 1000);

    // 初始化导航
    initNavigation();

    // 初始化传感器数据
    updateSensorDisplay();

    // 模拟数据更新
    setInterval(updateSensorData, 3000);

    // 初始化图表（延迟加载以确保Chart.js已加载）
    setTimeout(initCharts, 500);

    // 更新设备状态显示
    updateDeviceStatusDisplay();
});

// 时间更新函数
function updateTime() {
    const now = new Date();
    const timeString = now.toLocaleString('zh-CN', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
    });
    document.getElementById('currentTime').textContent = timeString;
    document.getElementById('updateTime').textContent = now.toLocaleTimeString('zh-CN');
}

// 导航初始化
function initNavigation() {
    const navButtons = document.querySelectorAll('.nav-btn');
    const sections = document.querySelectorAll('.section');

    navButtons.forEach(button => {
        button.addEventListener('click', function() {
            // 移除所有活动状态
            navButtons.forEach(btn => btn.classList.remove('active'));
            sections.forEach(section => section.classList.remove('active'));

            // 添加当前活动状态
            this.classList.add('active');
            const sectionId = this.getAttribute('data-section');
            document.getElementById(sectionId).classList.add('active');

            // 如果切换到图表页面，更新图表
            if (sectionId === 'charts') {
                setTimeout(updateCharts, 100);
            }
        });
    });
}

// 传感器数据更新（模拟）
function updateSensorData() {
    // 模拟数据波动
    sensorData.temperature = generateRandomValue(sensorData.temperature, 18, 32, 0.5);
    sensorData.humidity = generateRandomValue(sensorData.humidity, 40, 80, 1);
    sensorData.soilMoisture = generateRandomValue(sensorData.soilMoisture, 30, 90, 1);
    sensorData.lightIntensity = generateRandomValue(sensorData.lightIntensity, 500, 6000, 100);
    sensorData.soilPH = generateRandomValue(sensorData.soilPH, 5.5, 7.5, 0.1);
    sensorData.co2Concentration = generateRandomValue(sensorData.co2Concentration, 400, 1200, 20);

    // 更新显示
    updateSensorDisplay();

    // 检查预警条件
    checkAlerts();
}

// 生成随机值（在合理范围内波动）
function generateRandomValue(currentValue, min, max, maxChange) {
    const change = (Math.random() - 0.5) * 2 * maxChange;
    let newValue = currentValue + change;

    // 限制在范围内
    if (newValue < min) newValue = min;
    if (newValue > max) newValue = max;

    return Math.round(newValue * 10) / 10; // 保留一位小数
}

// 更新传感器显示
function updateSensorDisplay() {
    // 更新数值显示
    document.getElementById('temperature').textContent = sensorData.temperature.toFixed(1);
    document.getElementById('humidity').textContent = sensorData.humidity.toFixed(0);
    document.getElementById('soilMoisture').textContent = sensorData.soilMoisture.toFixed(0);
    document.getElementById('lightIntensity').textContent = sensorData.lightIntensity.toFixed(0);
    document.getElementById('soilPH').textContent = sensorData.soilPH.toFixed(1);
    document.getElementById('co2Concentration').textContent = sensorData.co2Concentration.toFixed(0);

    // 添加更新动画
    const sensorCards = document.querySelectorAll('.sensor-card');
    sensorCards.forEach(card => {
        card.classList.add('data-update');
        setTimeout(() => card.classList.remove('data-update'), 500);
    });

    // 更新颜色指示
    updateSensorColors();
}

// 更新传感器卡片颜色（根据数值是否在适宜范围内）
function updateSensorColors() {
    const temperatureEl = document.getElementById('temperature').closest('.sensor-card');
    const humidityEl = document.getElementById('humidity').closest('.sensor-card');
    const soilMoistureEl = document.getElementById('soilMoisture').closest('.sensor-card');
    const lightEl = document.getElementById('lightIntensity').closest('.sensor-card');
    const phEl = document.getElementById('soilPH').closest('.sensor-card');
    const co2El = document.getElementById('co2Concentration').closest('.sensor-card');

    // 温度检查 (18-28°C)
    if (sensorData.temperature < 18 || sensorData.temperature > 28) {
        temperatureEl.style.borderTopColor = '#e74c3c';
    } else {
        temperatureEl.style.borderTopColor = '#4caf50';
    }

    // 湿度检查 (40-70%)
    if (sensorData.humidity < 40 || sensorData.humidity > 70) {
        humidityEl.style.borderTopColor = '#e74c3c';
    } else {
        humidityEl.style.borderTopColor = '#4caf50';
    }

    // 土壤湿度检查 (50-80%)
    if (sensorData.soilMoisture < 50 || sensorData.soilMoisture > 80) {
        soilMoistureEl.style.borderTopColor = '#e74c3c';
    } else {
        soilMoistureEl.style.borderTopColor = '#4caf50';
    }

    // 光照检查 (1000-5000 lux)
    if (sensorData.lightIntensity < 1000 || sensorData.lightIntensity > 5000) {
        lightEl.style.borderTopColor = '#e74c3c';
    } else {
        lightEl.style.borderTopColor = '#4caf50';
    }

    // pH检查 (6.0-7.0)
    if (sensorData.soilPH < 6.0 || sensorData.soilPH > 7.0) {
        phEl.style.borderTopColor = '#e74c3c';
    } else {
        phEl.style.borderTopColor = '#4caf50';
    }

    // CO2检查 (400-1000 ppm)
    if (sensorData.co2Concentration < 400 || sensorData.co2Concentration > 1000) {
        co2El.style.borderTopColor = '#e74c3c';
    } else {
        co2El.style.borderTopColor = '#4caf50';
    }
}

// 初始化图表
function initCharts() {
    // 检查Chart.js是否已加载
    if (typeof Chart === 'undefined') {
        console.log('Chart.js未加载，使用备用方案');
        return;
    }

    // 生成24小时数据
    const hours = Array.from({length: 24}, (_, i) => `${i}:00`);
    const tempData = generateChartData(15, 30, 24);
    const humidityData = generateChartData(40, 80, 24);
    const soilData = generateChartData(40, 90, 24);
    const lightData = generateChartData(0, 8000, 24);
    const co2Data = generateChartData(400, 1200, 24);
    const phData = generateChartData(5.5, 7.5, 24);

    // 温湿度图表
    const ctx1 = document.getElementById('tempHumidityChart').getContext('2d');
    tempHumidityChart = new Chart(ctx1, {
        type: 'line',
        data: {
            labels: hours,
            datasets: [
                {
                    label: '温度 (°C)',
                    data: tempData,
                    borderColor: '#e74c3c',
                    backgroundColor: 'rgba(231, 76, 60, 0.1)',
                    tension: 0.4,
                    fill: true
                },
                {
                    label: '湿度 (%)',
                    data: humidityData,
                    borderColor: '#3498db',
                    backgroundColor: 'rgba(52, 152, 219, 0.1)',
                    tension: 0.4,
                    fill: true
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    position: 'top',
                }
            },
            scales: {
                y: {
                    beginAtZero: false
                }
            }
        }
    });

    // 土壤湿度与光照图表
    const ctx2 = document.getElementById('soilLightChart').getContext('2d');
    soilLightChart = new Chart(ctx2, {
        type: 'line',
        data: {
            labels: hours,
            datasets: [
                {
                    label: '土壤湿度 (%)',
                    data: soilData,
                    borderColor: '#8b4513',
                    backgroundColor: 'rgba(139, 69, 19, 0.1)',
                    tension: 0.4,
                    fill: true,
                    yAxisID: 'y'
                },
                {
                    label: '光照 (lux)',
                    data: lightData,
                    borderColor: '#f39c12',
                    backgroundColor: 'rgba(243, 156, 18, 0.1)',
                    tension: 0.4,
                    fill: true,
                    yAxisID: 'y1'
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    position: 'top',
                }
            },
            scales: {
                y: {
                    type: 'linear',
                    display: true,
                    position: 'left',
                    title: {
                        display: true,
                        text: '土壤湿度 (%)'
                    }
                },
                y1: {
                    type: 'linear',
                    display: true,
                    position: 'right',
                    title: {
                        display: true,
                        text: '光照 (lux)'
                    },
                    grid: {
                        drawOnChartArea: false,
                    },
                }
            }
        }
    });

    // CO2图表
    const ctx3 = document.getElementById('co2Chart').getContext('2d');
    co2Chart = new Chart(ctx3, {
        type: 'bar',
        data: {
            labels: hours,
            datasets: [{
                label: 'CO₂浓度 (ppm)',
                data: co2Data,
                backgroundColor: 'rgba(26, 188, 156, 0.6)',
                borderColor: '#1abc9c',
                borderWidth: 1
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    position: 'top',
                }
            },
            scales: {
                y: {
                    beginAtZero: false,
                    min: 300
                }
            }
        }
    });

    // pH图表
    const ctx4 = document.getElementById('phChart').getContext('2d');
    phChart = new Chart(ctx4, {
        type: 'line',
        data: {
            labels: hours,
            datasets: [{
                label: 'pH值',
                data: phData,
                borderColor: '#9b59b6',
                backgroundColor: 'rgba(155, 89, 182, 0.1)',
                tension: 0.4,
                fill: true,
                pointRadius: 3
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    position: 'top',
                }
            },
            scales: {
                y: {
                    beginAtZero: false,
                    min: 5,
                    max: 8,
                    ticks: {
                        stepSize: 0.5
                    }
                }
            }
        }
    });
}

// 生成图表数据
function generateChartData(min, max, count) {
    const data = [];
    let value = (min + max) / 2;

    for (let i = 0; i < count; i++) {
        const change = (Math.random() - 0.5) * (max - min) * 0.1;
        value += change;
        value = Math.max(min, Math.min(max, value));
        data.push(Math.round(value * 10) / 10);
    }

    return data;
}

// 更新图表数据
function updateCharts() {
    if (!tempHumidityChart) return;

    const hours = Array.from({length: 24}, (_, i) => `${i}:00`);
    const tempData = generateChartData(15, 30, 24);
    const humidityData = generateChartData(40, 80, 24);
    const soilData = generateChartData(40, 90, 24);
    const lightData = generateChartData(0, 8000, 24);
    const co2Data = generateChartData(400, 1200, 24);
    const phData = generateChartData(5.5, 7.5, 24);

    // 更新图表数据
    tempHumidityChart.data.datasets[0].data = tempData;
    tempHumidityChart.data.datasets[1].data = humidityData;
    tempHumidityChart.update();

    soilLightChart.data.datasets[0].data = soilData;
    soilLightChart.data.datasets[1].data = lightData;
    soilLightChart.update();

    co2Chart.data.datasets[0].data = co2Data;
    co2Chart.update();

    phChart.data.datasets[0].data = phData;
    phChart.update();
}

// 检查预警条件
function checkAlerts() {
    // 这里可以添加预警逻辑
    // 目前只是示例，实际应用中可以根据阈值生成预警
}

// 设备控制函数
function toggleDevice(device, state) {
    deviceStatus[device] = state;
    updateDeviceStatusDisplay();

    // 显示操作反馈
    const statusText = state ? '已开启' : '已关闭';
    const deviceNames = {
        irrigation: '灌溉系统',
        shade: '遮阳系统',
        ventilation: '通风系统',
        light: '补光系统',
        ai: 'AI智能模式',
        notification: '远程通知'
    };

    // 创建临时提示
    showToast(`${deviceNames[device]} ${statusText}`);
}

// 更新设备状态显示
function updateDeviceStatusDisplay() {
    const statusElements = {
        irrigation: document.getElementById('irrigationStatus'),
        shade: document.getElementById('shadeStatus'),
        ventilation: document.getElementById('ventilationStatus'),
        light: document.getElementById('lightStatus'),
        ai: document.getElementById('aiStatus'),
        notification: document.getElementById('notificationStatus')
    };

    const statusTexts = {
        irrigation: ['运行中', '关闭'],
        shade: ['展开', '收起'],
        ventilation: ['运行中', '关闭'],
        light: ['开启', '关闭'],
        ai: ['已启用', '已禁用'],
        notification: ['已启用', '已禁用']
    };

    Object.keys(deviceStatus).forEach(device => {
        const element = statusElements[device];
        if (element) {
            const isOn = deviceStatus[device];
            element.textContent = statusTexts[device][isOn ? 0 : 1];
            element.className = `status-indicator ${isOn ? 'on' : 'off'}`;
        }
    });
}

// 显示提示信息
function showToast(message) {
    // 移除现有的toast
    const existingToast = document.querySelector('.toast');
    if (existingToast) {
        existingToast.remove();
    }

    // 创建新的toast
    const toast = document.createElement('div');
    toast.className = 'toast';
    toast.textContent = message;
    toast.style.cssText = `
        position: fixed;
        bottom: 30px;
        left: 50%;
        transform: translateX(-50%);
        background-color: rgba(0, 0, 0, 0.8);
        color: white;
        padding: 12px 24px;
        border-radius: 25px;
        font-size: 14px;
        z-index: 1000;
        animation: fadeInOut 2s ease;
    `;

    document.body.appendChild(toast);

    // 2秒后移除
    setTimeout(() => {
        toast.remove();
    }, 2000);
}

// 添加CSS动画
const style = document.createElement('style');
style.textContent = `
    @keyframes fadeInOut {
        0% { opacity: 0; transform: translateX(-50%) translateY(20px); }
        20% { opacity: 1; transform: translateX(-50%) translateY(0); }
        80% { opacity: 1; transform: translateX(-50%) translateY(0); }
        100% { opacity: 0; transform: translateX(-50%) translateY(-20px); }
    }
`;
document.head.appendChild(style);

// 模拟AI决策（可选功能）
function simulateAIDecision() {
    if (!deviceStatus.ai) return;

    // 根据传感器数据自动决策
    if (sensorData.soilMoisture < 50 && !deviceStatus.irrigation) {
        toggleDevice('irrigation', true);
        showToast('AI决策：土壤湿度低，自动开启灌溉');
    }

    if (sensorData.temperature > 28 && !deviceStatus.ventilation) {
        toggleDevice('ventilation', true);
        showToast('AI决策：温度过高，自动开启通风');
    }

    if (sensorData.lightIntensity > 5000 && !deviceStatus.shade) {
        toggleDevice('shade', true);
        showToast('AI决策：光照过强，自动展开遮阳');
    }
}

// 每10秒执行一次AI决策
setInterval(simulateAIDecision, 10000);

// 页面加载完成后的初始化
console.log('校园微农园智能监测系统已启动');
console.log('系统功能：');
console.log('1. 实时传感器数据监测');
console.log('2. 历史数据图表展示');
console.log('3. 设备状态管理');
console.log('4. 预警信息通知');
console.log('5. 远程设备控制');
console.log('6. AI智能决策');
