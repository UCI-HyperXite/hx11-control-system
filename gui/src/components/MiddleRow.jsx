import React from "react";
import { Card } from "./Card";

export function MiddleRow({ telemetry }) {
	const middleRowStyle = {
		display: "flex",
		gap: 18,
        marginTop: 30,
		marginBottom: 18,
		flexShrink: 0,
	};

	return (
		<div style={middleRowStyle}>
			<Card
				title="Pressure" style={{ height: 270, width: 240, padding: 20, flexShrink: 0, paddingTop: 10}}>
				<div className="INA219Table">
					<table style = {{marginTop: -8, fontSize: 13.5, fontWeight: "bold"}}>
						<tr>
							<th style={{ width: 100 }}>INA219</th>
							<th style={{ width: 75 }}>Min</th>
						</tr>
                        <tr>
                            <td>vbus1: </td>
                            <td>{telemetry.vbus1} mV</td>
                        </tr>
                        <tr>
                            <td>vShunt1: </td>
                            <td>{telemetry.vShunt1} mV</td>
                        </tr>
                        <tr>
                            <td>current1: </td>
                            <td>{telemetry.current1} mA</td>
                        </tr>
                        <tr>
                            <td>power1: </td>
                            <td>{telemetry.power1} mW</td>
                        </tr>
                        <tr>
                            <td>vbus2: </td>
                            <td>{telemetry.vbus2} mV</td>
                        </tr>
                        <tr>
                            <td>vShunt2: </td>
                            <td>{telemetry.vShunt2} mV</td>
                        </tr>
                        <tr>
                            <td>current2: </td>
                            <td>{telemetry.current2} mA</td>
                        </tr>
                        <tr>
                            <td>power2: </td>
                            <td>{telemetry.power2} mW</td>
                        </tr>

					</table>
				</div>
			</Card>

			<Card
				title="LIM Temperature"
				style={{
			        height: 270, width: 260,flexShrink: 0, paddingTop: 10}}>
				<div>
					<div className="TempTable">
                        <table style={{marginTop: -7, marginLeft: -10, fontSize: 16, fontWeight: "bold", lineHeight: 2.5 }}>
                            <tr>
                                <th style={{fontSize: 11, width: 30 }}>Therm#</th>
                                <th style={{fontSize: 13, width: 80 }}>Temp</th>
                                <th style={{fontSize: 11, width: 30 }}>Therm#</th>
                                <th style={{fontSize: 13, width: 80 }}>Temp</th>
                            </tr>
                            <tr>
                                <td>1 </td>
                                <td>{telemetry.therm1} °C</td>
                                <td>5 </td>
                                <td>{telemetry.therm5} °C</td>
                            </tr>
                            <tr>
                                <td>2 </td>
                                <td>{telemetry.therm2} °C</td>
                                <td>6 </td>
                                <td>{telemetry.therm6} °C</td>
                            </tr>
                            <tr>
                                <td>3 </td>
                                <td>{telemetry.therm3} °C</td>
                                <td>7 </td>
                                <td>{telemetry.therm7} °C</td>
                            </tr>
                            <tr>
                                <td>4 </td>
                                <td>{telemetry.therm4} °C</td>
                                 <td>8 </td>
                                 <td>{telemetry.therm8} °C</td>
                            </tr>

                        </table>
                    </div>
					
				</div>
			</Card>
		</div>
	);
}
