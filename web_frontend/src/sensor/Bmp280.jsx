import axios from 'axios';
import { useState } from 'react';

const Bmp280 = () => {
    const [bmpData, setBmpData] = useState(null);
    
    const get_bmp280 = () => {
        axios.get('/api/bmp280').then(response => {
            setBmpData(response.data);
        });
    };

    return (
        <div>
            <button onClick={get_bmp280}>bmp280 데이터 불러오기</button>
            {bmpData && (
                <div>
                    <h2>불러온 데이터:</h2>
                    <pre>{JSON.stringify(bmpData, null, 2)}</pre>
                </div>
            )}
        </div>
    )
}

export default Bmp280