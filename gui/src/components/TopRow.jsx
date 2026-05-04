
import React, {useState, useRef, useEffect} from "react";
import { Card } from "./Card";


export function TopRow({ telemetry, consoleLogs = [] }) {

	function thermColor(temp) {
		const t = parseFloat(temp);
		if (t >= 70) return "#ff6868";
		if (t >= 60) return "#ff8972";  
		if (t >= 50) return "#fcae7a";
		if (t >= 40) return "#fff5b9";
		if (t >= 30) return "#ffffd7";
		if (t >= 15) return "#f0f0f0";
		return "#bdf1ff";                
	}
	return (
		<div style={{ 
			display: "flex", 
			gap: "1.25vw", 
			alignItems: "flex-start",
			width: "100%",
			}}>

			{/* LEFT COLUMN */}
			<div style={{ 
				display: "flex", 
				flexDirection: "column", 
				gap: "1.5vw", 
				flexShrink: 0 }}>

				{/* Top two cards */}
				<div style={{ display: "flex", gap: "1.25vw" }}>
					<Card style={{ 
						height: "12vw", 
						width: "14vw", 
						flexShrink: 0, 
						fontWeight: "bold", 
						fontSize: "1.2vw", 
						lineHeight: 2, 
						paddingLeft: "1.2vw",
						paddingTop: "1.2vw" }}>
						<div>Time: {telemetry.time}</div>
						<div>Distance: {telemetry.distance} cm</div>
						<div>Position: {telemetry.position}</div>
						<div>Speed: {telemetry.speed}</div>
					</Card>

					<Card title="Rotation" 
						style={{ 
							width: "17.5vw", 
							flexShrink: 0,
							paddingBottom: "0.5vw"
						}}>
					<div className="rotTable" style={{ marginTop: "-2vw", display: "flex", alignItems: "center", marginRight: "-2vw", }}>
						<table style={{ fontSize: "1.2vw", fontWeight: "bold" }}>
							<thead>
								<tr><th>Gyro</th></tr>
							</thead>
							<tbody>
								<tr>
									<td>
										<div>x: {telemetry.roll} rad/s</div>
										<div>y: {telemetry.pitch} rad/s</div>
									</td>
								</tr>
							</tbody>
						</table>

						{/* Gyro visualizer completely vibe coded :> */} 
						{(() => {
							const cx = 50, cy = 50, r = 40;
							const clamp = (v, min, max) => Math.max(min, Math.min(max, v));

							const xNorm = clamp(parseFloat(telemetry.roll), -2, 2) / 2; // roll:  -1 to 1
							const yNorm = clamp(parseFloat(telemetry.pitch), -2, 2) / 2;  // pitch: -1 to 1

							const rollAngle = xNorm * (Math.PI / 2);  // ±90° tilt of major axis
							const ry = r * Math.abs(yNorm);            // minor axis: 0=flat line, r=full circle
							const rollDeg = xNorm * 90;               // SVG x-rotation in degrees

							// Endpoints of the major axis (where front/back split occurs)
							const P1x = cx + r * Math.cos(rollAngle);
							const P1y = cy + r * Math.sin(rollAngle);
							const P2x = cx - r * Math.cos(rollAngle);
							const P2y = cy - r * Math.sin(rollAngle);

							// Pitch forward (yNorm > 0): bottom arc faces viewer → sweep=1 (clockwise in screen)
							const frontSweep = yNorm >= 0 ? 1 : 0;
							const backSweep  = yNorm >= 0 ? 0 : 1;

							const arcArgs = `${r} ${ry.toFixed(2)} ${rollDeg.toFixed(1)} 0`;
							const frontArc = `M ${P1x.toFixed(2)} ${P1y.toFixed(2)} A ${arcArgs} ${frontSweep} ${P2x.toFixed(2)} ${P2y.toFixed(2)}`;
							const backArc  = `M ${P1x.toFixed(2)} ${P1y.toFixed(2)} A ${arcArgs} ${backSweep}  ${P2x.toFixed(2)} ${P2y.toFixed(2)}`;
							
							// Red dot at midpoint of the front arc
							const dotX = yNorm >= 0
								? cx - ry * Math.sin(rollAngle)
								: cx + ry * Math.sin(rollAngle);
							const dotY = yNorm >= 0
								? cy + ry * Math.cos(rollAngle)
								: cy - ry * Math.cos(rollAngle);

							const gradX = 50 - xNorm * 20;
							const gradY = 50 - yNorm * 20;

							return (
								<svg viewBox="0 0 100 100" style={{ width: "8vw",flexShrink: 0, marginTop: "0.1vw" }}>
								<defs>
									<clipPath id="gyroClip">
									<circle cx={cx} cy={cy} r={r - 1} />
									</clipPath>
									<radialGradient id="sphereGrad" cx={`${gradX}%`} cy={`${gradY}%`} r="60%">
									<stop offset="0%"   stopColor="#4a3f6b" stopOpacity="0.0" />
									<stop offset="60%"  stopColor="#1a1428" stopOpacity="0.5" />
									<stop offset="100%" stopColor="#0a0810" stopOpacity="0.9" />
									</radialGradient>
									<radialGradient id="sphereShine" cx={`${gradX}%`} cy={`${gradY}%`} r="40%">
									<stop offset="0%"   stopColor="white" stopOpacity="0.15" />
									<stop offset="100%" stopColor="white" stopOpacity="0" />
									</radialGradient>
								</defs>

								{/* Sphere base + shading */}
								<circle cx={cx} cy={cy} r={r} fill="#1e1a2e" stroke="#444" strokeWidth="2" />
								<circle cx={cx} cy={cy} r={r} fill="url(#sphereGrad)" />

								{/* Back half — dashed, dim */}
								<path d={backArc}
								stroke="#005f7f" strokeWidth="1.5" strokeDasharray="1 2"
								fill="none"/>

								{/* Front half — solid, bright */}
								<path d={frontArc}
								stroke="#00e5ff" strokeWidth="2.5"
								fill="none"/>

								{/* Shine */}
								<circle cx={cx} cy={cy} r={r} fill="url(#sphereShine)" clipPath="url(#gyroClip)" />

								{/* Crosshair */}
								<line x1={cx-6} y1={cy} x2={cx+6} y2={cy} stroke="white" strokeWidth="1" opacity="0.4" />
								<line x1={cx} y1={cy-6} x2={cx} y2={cy+6} stroke="white" strokeWidth="1" opacity="0.4" />

								{/* Red dot at front-arc midpoint */}
								<circle cx={dotX} cy={dotY} r={3} fill="red"/>

								</svg>
							);
							})()}
					</div>
					</Card>
				</div>

				{/* Middle cards below */}
				<div style={{ display: "flex", gap: "1.5vw" }}>
					<Card title="Pressure" style={{ width: "13vw", flexShrink: 0 }}>
						<div style={{ marginTop: "-1.5vw" }}>
							<table style={{ fontSize: "1.2vw", fontWeight: "bold", width: "100%", marginTop: "1vw"}} className="INA219Table">
								<thead>
									<tr style={{fontSize: "1vw"}}>
										<th style={{ width: "55%" }}>INA219</th>
										<th style={{ width: "45%" }}>Value</th>
									</tr>
								</thead>
								<tbody>
									{[
										["up:", `${telemetry.current1} PSI`],
										["down:", `${telemetry.current2} PSI`],
									].map(([label, value]) => (
										<tr key={label}><td>{label}</td><td>{value}</td></tr>
									))}
								</tbody>
							</table>
							{/* <table style={{ fontSize: "1.2vw", fontWeight: "bold", width: "100%", marginTop: "1.7vw" }} className="INA219Table">
									<thead>
										<tr style={{fontSize: "1.3vw"}}>
											<th style={{ width: "55%" }}>INA260</th>
											<th style={{ width: "45%" }}>Value</th>
										</tr>
									</thead>
									<tbody>
  										<tr><td>LV current:</td><td>{telemetry.current1} mA</td></tr>
									</tbody>
							</table> */}
						</div>
					</Card>

					<Card title="LIM Temperature" style={{ width: "17vw", flexShrink: 0}}>
						<table style={{ width: "100%" }} className="TempTable">
							<thead>
								<tr style = {{fontSize: "1vw"}}>
									<th style={{ width: "14%" }}>Th#</th>
									<th style={{ width: "36%" }}>Temp</th>
									<th style={{ width: "14%" }}>Th#</th>
									<th style={{ width: "36%" }}>Temp</th>
								</tr>
							</thead>
							<tbody style = {{fontSize: "1.1vw"}}>
								{[1, 2, 3, 4].map(i => (
								<tr key={i}>
									<td>{i}</td>
									<td style={{ backgroundColor: thermColor(telemetry[`therm${i}`]) }}>
										{telemetry[`therm${i}`]} °C
									</td>
									<td>{i + 4}</td>
									<td style={{ backgroundColor: thermColor(telemetry[`therm${i + 4}`]) }}>
										{telemetry[`therm${i + 4}`]} °C
									</td>
								</tr>
							))}
							</tbody>
						</table>
					</Card>
				</div>

			</div>

			{/* RIGHT COLUMN — Other Info spans full height */}
			<Card title="Other Info" style={{ width: "36vw", flexShrink: 0,}}>
				<div style={{
					background: "#2f2740", 
					color: "#f0e6f2", 
					borderRadius: "0.833vw",
					padding: "0 1vw", 
					fontWeight: "bold", 
					fontSize: "0.88vw", 
					lineHeight: "2.95vw",
					
				}}>
					<div className="OtherInfoTable">
						<table style={{ width: "100%", }}>
							<thead>
								<tr>
									<th style={{ width: "40%" }}></th>
									<th style={{ width: "15%" }}>Min</th>
									<th style={{ width: "15%" }}>Value</th>
									<th style={{ width: "15%" }}>Max</th>
									<th style={{ width: "15%" }}>Unit</th>
								</tr>
							</thead>
							<tbody>
								{[
									
									["HV Batt Volt (avg):", telemetry.battVoltage, "V"],
									["HV Batt Current:", telemetry.battCurrent, "A"],
									["HV Batt SoC:", telemetry.battSoC, "%"],
									["LIM Voltage:", telemetry.limVoltage, "V"],
									["LIM Current:", telemetry.limCurrent, "A"],
									["LV Batt Voltage ", telemetry.lvbattVoltage, "V"],
									
									["Batt Temp:", telemetry.battTemp, "°C"],
									["IMD Status:", telemetry.imdStatus, ""],
								].map(([label, value, unit]) => (
									<tr key={label}>
										<td>{label}</td>
										<td></td>
										<td>{value}</td>
										<td></td>
										<td>{unit}</td>
									</tr>
								))}
							</tbody>
						</table>
					</div>
				</div>
			</Card>

			<Card title="Console" style={{ 
				
				width: "27vw", 
				flexShrink: 0,
				display: "flex",
				flexDirection: "column",
				maxHeight: "50vw"
				}}>
							<div style={{
									height: "30.3vw",
									background: "#2f2740",
									color: "#f0e6f2",
									borderRadius: "0.993vw",
									overflowY: "auto",
									display: "flex",
									flexDirection: "column",
									position: "relative",
									overflow: "hidden",
									justifyContent: "center",
									alignItems: "center",
									fontSize: "1.1vw",
									gap: "0.333vw"
								}}
							>
								<div
									style={{
										position: "absolute",
										top: "1.25vw",
										left: "1.25vw",
										fontSize: "1.868vw"
									}}
								>
									&gt;&gt;
								</div>
			
								<div
									style={{
									marginTop: "6vw",
									flex: 1,
									overflowY: "auto",
									padding: "0 0.993vw 0.667vw 0.993vw",
									display: "flex",
									flexDirection: "column",
									gap: 4,
								}}>
								{consoleLogs.length === 0 ? (
			
								<div style={{ color: "#a080b0", fontSize: "1.1vw", fontStyle: "italic" }}>
									Waiting for connection...
								</div>
			
								) : (
								[...consoleLogs].reverse().map((log, i) => (
								<div
									key={i}
									style={{
										fontSize: "1.1vw",
										fontFamily: "monospace",
										color: log.includes("✓") ? "#7effa0"
											: log.includes("failed") || log.includes("error") ? "#ff8080"
											: "#f0e6f2",
										borderBottom: "1px solid #3d3050",
										paddingBottom: "0.5vw",
										wordBreak: "break-word",
									}}
								>
									{log}
								</div>
								))
								)}
								{/* <div ref={logEndRef} /> */}
								</div>
							</div>
						</Card>
						

		</div>
	);
}