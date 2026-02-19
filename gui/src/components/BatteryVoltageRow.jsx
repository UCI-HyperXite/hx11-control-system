import React from "react";
import { Card } from "./Card";

export function BatteryVoltageRow({ telemetry }) {
    const BatteryVoltageRowStyle = {
        display: "flex",
        gap: 18,
        marginTop: -10,
        marginBottom: 10,
        flexShrink: 0,
    };

    return (
        <div style={BatteryVoltageRowStyle}>
            <Card
                title="Current"
                style={{height: 450, width: 290,flexShrink: 0}}
            >
            </Card>

            <Card
                title="HV Battery"
                style={{
                    height: 450,
                    width: 730,
                    flexShrink: 0,
                }}
            >
                <div>
                    <div className="HVBatteryTable">
                        <table>
                            <tr style={{width: 60, fontSize: 12 }}>
                                <th>Pack</th>
                                <th>Cell 0</th>
                                <th>Cell 1</th>
                                <th>Cell 2</th>
                                <th>Cell 3</th>
                                <th>Cell 4</th>
                                <th>Cell 5</th>
                                <th>Cell 6</th>
                                <th>Cell 7</th>
                                <th>Cell 8</th>
                                <th>Cell 9</th>
                                <th>Cell 10</th>
                                <th>Cell 11</th>
                                <th>Cell 12</th>
                                <th>   Sum  </th>
                            </tr>
     
                            
                            {Array.from({ length: 12 }).map((_, rowIndex) => (
                            <tr key={rowIndex}>
                                {Array.from({ length: 15 }).map((_, colIndex) => {

                                const isHighlighted = rowIndex === 2 && colIndex === 5;
                                const isFirstColumn = colIndex === 0;
                                const isFirstRow = rowIndex === 0 && colIndex !== 0;

                                return (
                                    <td
                                    key={colIndex}
                                    style={{
                                        backgroundColor: isHighlighted ? "red" : isFirstRow ? "green" : undefined,
                                        fontWeight: isFirstColumn ? "bold" : "normal"
                                    }}
                                    >
                                    {isFirstColumn ? rowIndex + 1 : "X"}
                                    </td>
                                );
                                })}
                            </tr>
                            ))}
                            
                        </table>
                    </div>
                    
                </div>
            </Card>
        </div>
    );
}
