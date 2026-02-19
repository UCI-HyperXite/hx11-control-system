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
				title="Pressure" style={{ height: 270, width: 240, padding: 13, flexShrink: 0 }}>
				<div className="INA219Table">
					<table style = {{fontSize: 12}}>
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
			        height: 270, width: 260,flexShrink: 0}}>
				<div>
					<div className="TempTable">
                        <table style={{ fontSize: 13 }}>
                            <tr>
                                <th style={{ width: 60 }}>Therm #</th>
                                <th style={{ width: 80 }}>Temp</th>
                            </tr>
                            <tr>
                                <td>1 </td>
                                <td>{telemetry.therm1} °C</td>
                            </tr>
                            <tr>
                                <td>2 </td>
                                <td>{telemetry.therm2} °C</td>
                            </tr>
                            <tr>
                                <td>3 </td>
                                <td>{telemetry.therm3} °C</td>
                            </tr>
                            <tr>
                                <td>4 </td>
                                <td>{telemetry.therm4} °C</td>
                            </tr>
                            <tr>
                                <td>5 </td>
                                <td>{telemetry.therm5} °C</td>
                            </tr>
                            <tr>
                                <td>6 </td>
                                <td>{telemetry.therm6} °C</td>
                            </tr>
                            <tr>
                                <td>7 </td>
                                <td>{telemetry.therm7} °C</td>
                            </tr>
                            <tr>
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
