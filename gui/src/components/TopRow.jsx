import React from "react";
import { Card } from "./Card";

export function TopRow({ telemetry }) {
	const topRowStyle = {
		display: "flex",
		gap: 18,
		marginBottom: -300,
		flex: 1,
	};

	return (
		<div style={topRowStyle}>
			{/* left-top Pressure */}
			<Card title=""
				style={{height: 150, width: 180, flexShrink: 0, paddingTop: 11, fontWeight: "bold"}}
			>
				<div style={{ lineHeight: 2, display: "flex" }}>Time: {telemetry.time}</div>
				<div style={{ lineHeight: 2, display: "flex" }}>Distance: {telemetry.distance} cm</div>
				<div style={{ lineHeight: 2, display: "flex" }}>Position: {telemetry.position}</div>
				<div style={{ lineHeight: 2, display: "flex" }}>Speed: {telemetry.speed}</div>
			</Card>

			{/* top-center Rotation */}
			<Card title="Rotation" style={{ height: 150, width: 320, flexShrink: 0, fontWeight: "bold", paddingTop: 10, fontSize: 16}}>
				<div style={{marginTop: -8, display: "flex", textAlign: "left"}}>
					<div>
						<div>Accel:</div>
						<div style={{ marginLeft: 15, display: "flex", gap:20}}>
							<div>X: {telemetry.accelerationx}</div>
							<div>Y: {telemetry.accelerationy}</div>
							<div>Z: {telemetry.accelerationz} m/s²</div>
						</div>
					</div>
				</div>
				<div style={{marginTop: 8, textAlign: "left"}}>
						<div>Gyro: </div>
						<div style={{ marginLeft: 15, display: "flex", gap:20}}>
							<div>X: {telemetry.gyrox}</div>
							<div>Y: {telemetry.gyroy}</div>
							<div> Z: {telemetry.gyroz} rad/s</div>
						</div>
				</div>
			</Card>

			{/* right-top Other Info */}
			<Card
				title="OTHER INFO"
				style={{ height: 440, width: 500, flexShrink: 0, display: "flex", flexDirection: "column", padding : 12 }}
			>
				<div
					style={{
						height: 380,
						background: "#2f2740",
						color: "#f0e6f2",
						borderRadius: 12,
						flexShrink: 0,
						paddingLeft: 15,
						paddingRight: 15,
						fontWeight: "bold",
						fontSize: 16.5,
						lineHeight: 2.5,
					}}
				>
					
					<div className="OtherInfoTable">
                        <table>
                            <tr>
                                <th style={{ width: 150 }}></th>
								<th style={{ width: 75 }}>Min</th>
                                <th style={{ width: 75 }}>Value</th>
								<th style={{ width: 75 }}>Max</th>
								<th style={{ width: 75 }}>Unit</th>

                            </tr>
                            <tr>
                                <td>LIM Voltage:</td>
								<td></td>
                                <td>{telemetry.limVoltage}</td>
								<td></td>
								<td>V</td>
                            </tr>
							<tr>
                                <td>LIM Current:</td>
								<td></td>
                                <td>{telemetry.limCurrent}</td>
								<td></td>
								<td>A</td>
                            </tr>
                            <tr>
                                <td>Batt Voltage:</td>
								<td></td>
                                <td>{telemetry.battVoltage}</td>
								<td></td>
								<td>V</td>
                            </tr>
							<tr>
                                <td>Batt Current:</td>
								<td></td>
                                <td>{telemetry.battCurrent}</td>
								<td></td>
								<td>A</td>
                            </tr>
							<tr>
                                <td>Batt SoC:</td>
								<td></td>
                                <td>{telemetry.battSoC}</td>
								<td></td>
								<td>%</td>
                            </tr>
							<tr>
                                <td>Batt Temp:</td>
								<td></td>
                                <td>{telemetry.battTemp}</td>
								<td></td>
								<td>°C</td>
                            </tr>
							<tr>
                                <td>IMD Status:</td>
								<td></td>
                                <td>{telemetry.imdStatus}</td>
								<td></td>
								<td></td>
                            </tr>

                        </table>
                    </div>
				</div>
			</Card>
		</div>
	);
}
