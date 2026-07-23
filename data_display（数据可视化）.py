"""
校园微农园智能监测系统 - 数据可视化脚本

功能：
1. 模拟传感器数据生成
2. 生成环境参数趋势图
3. 生成数据统计报表

依赖：
    pip install matplotlib pandas numpy

使用：
    python data_display.py

作者：（填写）
日期：2026年6月
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime, timedelta
import json
import os

# ==================== 配置 ====================
OUTPUT_DIR = "output"  # 输出目录
DAYS = 7  # 模拟天数
SAMPLE_INTERVAL = 10  # 采样间隔（分钟）

# 设置中文字体
plt.rcParams['font.sans-serif'] = ['SimHei', 'Microsoft YaHei', 'Arial']
plt.rcParams['axes.unicode_minus'] = False


def generate_sample_data(days=7, interval_minutes=10):
    """
    生成模拟传感器数据

    参数：
        days: 模拟天数
        interval_minutes: 采样间隔（分钟）

    返回：
        DataFrame: 包含所有传感器数据的时间序列
    """
    # 计算数据点数量
    points_per_day = 24 * 60 // interval_minutes
    total_points = days * points_per_day

    # 生成时间序列
    start_time = datetime.now() - timedelta(days=days)
    timestamps = [start_time + timedelta(minutes=i * interval_minutes)
                  for i in range(total_points)]

    # 生成模拟数据（带日变化和随机噪声）
    t = np.linspace(0, days * 2 * np.pi, total_points)

    # 温度：日变化 + 随机波动
    temperature = 25 + 8 * np.sin(t / points_per_day * 2 * np.pi - np.pi/2) + \
                  np.random.normal(0, 0.5, total_points)

    # 湿度：与温度反相关
    humidity = 65 - 15 * np.sin(t / points_per_day * 2 * np.pi - np.pi/2) + \
               np.random.normal(0, 1, total_points)
    humidity = np.clip(humidity, 30, 95)

    # 土壤湿度：缓慢下降 + 灌溉回升
    soil_moisture = np.zeros(total_points)
    soil_moisture[0] = 55
    for i in range(1, total_points):
        # 自然蒸发
        soil_moisture[i] = soil_moisture[i-1] - np.random.uniform(0.02, 0.05)
        # 灌溉模拟（当湿度低于30%时）
        if soil_moisture[i] < 30:
            soil_moisture[i] += np.random.uniform(25, 35)
        soil_moisture[i] += np.random.normal(0, 0.3)
    soil_moisture = np.clip(soil_moisture, 15, 85)

    # 光照强度：白天高、夜间低
    light_intensity = np.zeros(total_points)
    for i in range(total_points):
        hour = timestamps[i].hour
        if 6 <= hour <= 18:
            # 白天：正午最高
            base = 50000 * np.sin((hour - 6) / 12 * np.pi)
            light_intensity[i] = max(0, base + np.random.normal(0, 2000))
        else:
            # 夜间
            light_intensity[i] = np.random.uniform(0, 100)

    # pH值：缓慢变化
    ph = 6.8 + 0.3 * np.sin(t / points_per_day * 4 * np.pi) + \
         np.random.normal(0, 0.05, total_points)
    ph = np.clip(ph, 5.5, 8.0)

    # CO2浓度：白天低（光合作用）、夜间高
    co2 = np.zeros(total_points)
    for i in range(total_points):
        hour = timestamps[i].hour
        if 6 <= hour <= 18:
            co2[i] = 500 + np.random.normal(0, 30)
        else:
            co2[i] = 800 + np.random.normal(0, 50)
    co2 = np.clip(co2, 400, 1500)

    # 创建DataFrame
    df = pd.DataFrame({
        'timestamp': timestamps,
        'temperature': np.round(temperature, 1),
        'humidity': np.round(humidity, 1),
        'soil_moisture': np.round(soil_moisture, 1),
        'light_intensity': np.round(light_intensity, 0).astype(int),
        'ph': np.round(ph, 2),
        'co2': np.round(co2, 0).astype(int)
    })

    return df


def plot_temperature_humidity(df, save_path=None):
    """
    绘制温度和湿度趋势图
    """
    fig, ax1 = plt.subplots(figsize=(12, 6))

    # 温度曲线
    color = 'tab:red'
    ax1.set_xlabel('时间')
    ax1.set_ylabel('温度 (℃)', color=color)
    ax1.plot(df['timestamp'], df['temperature'], color=color, linewidth=1, label='温度')
    ax1.tick_params(axis='y', labelcolor=color)
    ax1.set_ylim(10, 40)

    # 湿度曲线（双Y轴）
    ax2 = ax1.twinx()
    color = 'tab:blue'
    ax2.set_ylabel('湿度 (%RH)', color=color)
    ax2.plot(df['timestamp'], df['humidity'], color=color, linewidth=1, label='湿度')
    ax2.tick_params(axis='y', labelcolor=color)
    ax2.set_ylim(30, 100)

    # 标题和图例
    plt.title('校园微农园 - 温湿度监测趋势图', fontsize=14, fontweight='bold')

    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2, loc='upper right')

    plt.grid(True, alpha=0.3)
    fig.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Chart saved: {save_path}")

    plt.show()


def plot_soil_moisture(df, save_path=None):
    """
    绘制土壤湿度趋势图
    """
    fig, ax = plt.subplots(figsize=(12, 5))

    ax.plot(df['timestamp'], df['soil_moisture'], color='brown', linewidth=1)
    ax.axhline(y=30, color='red', linestyle='--', label='灌溉阈值 (30%)')
    ax.axhline(y=60, color='green', linestyle='--', label='目标湿度 (60%)')

    ax.set_xlabel('时间')
    ax.set_ylabel('土壤湿度 (%)')
    ax.set_title('校园微农园 - 土壤湿度监测趋势图', fontsize=14, fontweight='bold')
    ax.set_ylim(15, 85)
    ax.legend()
    ax.grid(True, alpha=0.3)

    fig.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Chart saved: {save_path}")

    plt.show()


def plot_environment_overview(df, save_path=None):
    """
    绘制环境参数总览图（四合一）
    """
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))

    # 光照强度
    ax1 = axes[0, 0]
    ax1.fill_between(df['timestamp'], df['light_intensity'], alpha=0.3, color='orange')
    ax1.plot(df['timestamp'], df['light_intensity'], color='orange', linewidth=0.8)
    ax1.set_title('光照强度')
    ax1.set_ylabel('光照 (lux)')
    ax1.grid(True, alpha=0.3)

    # CO2浓度
    ax2 = axes[0, 1]
    ax2.plot(df['timestamp'], df['co2'], color='green', linewidth=0.8)
    ax2.axhline(y=1000, color='red', linestyle='--', alpha=0.5, label='上限 1000ppm')
    ax2.set_title('CO₂浓度')
    ax2.set_ylabel('CO₂ (ppm)')
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    # pH值
    ax3 = axes[1, 0]
    ax3.plot(df['timestamp'], df['ph'], color='purple', linewidth=0.8)
    ax3.axhspan(6.0, 7.5, alpha=0.2, color='green', label='适宜范围')
    ax3.set_title('土壤pH值')
    ax3.set_ylabel('pH')
    ax3.set_ylim(5.0, 8.5)
    ax3.legend()
    ax3.grid(True, alpha=0.3)

    # 综合对比
    ax4 = axes[1, 1]
    # 归一化显示
    temp_norm = (df['temperature'] - df['temperature'].min()) / (df['temperature'].max() - df['temperature'].min())
    humi_norm = (df['humidity'] - df['humidity'].min()) / (df['humidity'].max() - df['humidity'].min())
    soil_norm = (df['soil_moisture'] - df['soil_moisture'].min()) / (df['soil_moisture'].max() - df['soil_moisture'].min())

    ax4.plot(df['timestamp'], temp_norm, label='温度', linewidth=0.8)
    ax4.plot(df['timestamp'], humi_norm, label='湿度', linewidth=0.8)
    ax4.plot(df['timestamp'], soil_norm, label='土壤湿度', linewidth=0.8)
    ax4.set_title('环境参数综合对比（归一化）')
    ax4.legend()
    ax4.grid(True, alpha=0.3)

    plt.suptitle('校园微农园 - 环境参数监测总览', fontsize=16, fontweight='bold', y=1.02)
    fig.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Chart saved: {save_path}")

    plt.show()


def generate_statistics(df):
    """
    生成数据统计报表
    """
    stats = {
        '参数': ['温度 (C)', '湿度 (%RH)', '土壤湿度 (%)', '光照 (lux)', 'pH', 'CO2 (ppm)'],
        '最小值': [
            df['temperature'].min(),
            df['humidity'].min(),
            df['soil_moisture'].min(),
            df['light_intensity'].min(),
            df['ph'].min(),
            df['co2'].min()
        ],
        '最大值': [
            df['temperature'].max(),
            df['humidity'].max(),
            df['soil_moisture'].max(),
            df['light_intensity'].max(),
            df['ph'].max(),
            df['co2'].max()
        ],
        '平均值': [
            round(df['temperature'].mean(), 1),
            round(df['humidity'].mean(), 1),
            round(df['soil_moisture'].mean(), 1),
            round(df['light_intensity'].mean(), 0),
            round(df['ph'].mean(), 2),
            round(df['co2'].mean(), 0)
        ],
        '标准差': [
            round(df['temperature'].std(), 2),
            round(df['humidity'].std(), 2),
            round(df['soil_moisture'].std(), 2),
            round(df['light_intensity'].std(), 0),
            round(df['ph'].std(), 3),
            round(df['co2'].std(), 0)
        ]
    }

    stats_df = pd.DataFrame(stats)
    return stats_df


def export_to_csv(df, save_path):
    """
    导出数据到CSV文件
    """
    df.to_csv(save_path, index=False, encoding='utf-8-sig')
    print(f"Data exported: {save_path}")


def export_to_json(df, save_path):
    """
    导出数据到JSON文件
    """
    data = {
        'metadata': {
            'node_id': 'node_01',
            'location': '校园微农园',
            'start_time': df['timestamp'].iloc[0].isoformat(),
            'end_time': df['timestamp'].iloc[-1].isoformat(),
            'sample_count': len(df)
        },
        'data': df.to_dict(orient='records')
    }

    # 转换datetime为字符串
    for record in data['data']:
        record['timestamp'] = record['timestamp'].isoformat()

    with open(save_path, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

    print(f"Data exported: {save_path}")


def main():
    """
    主函数
    """
    print("=" * 50)
    print("校园微农园智能监测系统 - 数据可视化")
    print("=" * 50)

    # 创建输出目录
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    # 生成模拟数据
    print("\n[1/5] 生成模拟传感器数据...")
    df = generate_sample_data(days=DAYS, interval_minutes=SAMPLE_INTERVAL)
    print(f"  生成 {len(df)} 条数据记录")
    print(f"  时间范围: {df['timestamp'].iloc[0]} ~ {df['timestamp'].iloc[-1]}")

    # 生成统计报表
    print("\n[2/5] 生成数据统计报表...")
    stats = generate_statistics(df)
    print("\n" + stats.to_string(index=False))

    # 绘制图表
    print("\n[3/5] 绘制温湿度趋势图...")
    plot_temperature_humidity(df, os.path.join(OUTPUT_DIR, "temperature_humidity.png"))

    print("\n[4/5] 绘制土壤湿度趋势图...")
    plot_soil_moisture(df, os.path.join(OUTPUT_DIR, "soil_moisture.png"))

    print("\n[5/5] 绘制环境参数总览图...")
    plot_environment_overview(df, os.path.join(OUTPUT_DIR, "environment_overview.png"))

    # 导出数据
    print("\n[额外] 导出数据文件...")
    export_to_csv(df, os.path.join(OUTPUT_DIR, "sensor_data.csv"))
    export_to_json(df, os.path.join(OUTPUT_DIR, "sensor_data.json"))

    print("\n" + "=" * 50)
    print("处理完成！")
    print(f"输出目录: {os.path.abspath(OUTPUT_DIR)}")
    print("=" * 50)


if __name__ == "__main__":
    main()
