document.addEventListener('DOMContentLoaded', () => {
    fetch('data/data.txt')
        .then(response => response.text())
        .then(text => {
            const data = text.replace(/\r/g, '').trim().split('\n');
            const container = document.getElementById('data-container');
            data.reverse().forEach((item, index) => {
                if (item) {
                    const row = document.createElement('div');
                    row.className = 'row';
                    
                    const flexContainer = document.createElement('div');
                    flexContainer.style.display = 'flex';
                    flexContainer.style.justifyContent = 'space-between';
                    flexContainer.style.alignItems = 'center';

                    // Display the entire line of data
                    const label = document.createElement('label');
                    label.htmlFor = `checkbox-${data.length - index}`;
                    label.textContent = `Data ${data.length - index}: ${item}`;

                    const checkbox = document.createElement('input');
                    checkbox.type = 'checkbox';
                    checkbox.id = `checkbox-${data.length - index}`;
                    checkbox.name = 'data-checkbox';
                    checkbox.value = item;
                    checkbox.addEventListener('change', () => handleCheckboxChange(item, checkbox));

                    flexContainer.appendChild(label);
                    flexContainer.appendChild(checkbox);
                    row.appendChild(flexContainer);

                    container.appendChild(row);

                    // Add click event listener to the row for opening the modal
                    row.addEventListener('click', (event) => {
                        if (event.target.type !== 'checkbox') {
                            const dataParts = item.split(',');
                            const modalData = dataParts.slice(1, 2).join(', '); // Get data after the first comma and before the third comma
                            showModal(modalData); // Show only the specified segment in the modal
                        }
                    });
                }
            });
        })
        .catch(error => console.error('Error loading the data:', error));

    const handleCheckboxChange = (item, selectedCheckbox) => {
        document.querySelectorAll('input[type="checkbox"]').forEach(checkbox => {
            if (checkbox !== selectedCheckbox) {
                checkbox.checked = false;
            }
        });

        if (selectedCheckbox.checked) {
            const firstPart = item.split(',')[0]; // Send only the first part up to the first comma

            fetch('api/setbutton', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ data: firstPart }),
            })
            .then(response => response.json())
            .then(data => console.log('Success:', data))
            .catch(error => console.error('Error:', error));
        }
    };

    const showModal = (data) => {
        const modal = document.getElementById('myModal');
        const modalText = document.getElementById('modal-text');
        modalText.value = data; // Set the text area value to the specified data segment
        modal.style.display = "block";
    };

    // Modal close and button functionality
    const modal = document.getElementById('myModal');
    const span = document.getElementsByClassName('close')[0];
    span.onclick = function() {
        modal.style.display = "none";
    };
    window.onclick = function(event) {
        if (event.target == modal) {
            modal.style.display = "none";
        }
    };

    const replayBtn = document.getElementById('replay-btn');
    replayBtn.onclick = function() {
        const textBox = document.getElementById('modal-text');
        const textValue = textBox.value;

        fetch('api/replay', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ data: textValue }),
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            return response.json();
        })
        .then(data => console.log('Success:', data))
        .catch(error => console.error('Error:', error));

        modal.style.display = "none";
    };
});
