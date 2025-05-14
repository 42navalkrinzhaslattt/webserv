// Wait for the DOM to be fully loaded
document.addEventListener('DOMContentLoaded', function() {
    // Get the button and result elements
    const button = document.getElementById('js-test-button');
    const result = document.getElementById('js-test-result');
    
    // Add click event listener to the button
    if (button && result) {
        button.addEventListener('click', function() {
            // Change the result text
            result.textContent = 'JavaScript is working! Clicked at: ' + new Date().toLocaleTimeString();
            
            // Change the button color
            button.style.backgroundColor = getRandomColor();
        });
    }
    
    // Function to generate a random color
    function getRandomColor() {
        const letters = '0123456789ABCDEF';
        let color = '#';
        for (let i = 0; i < 6; i++) {
            color += letters[Math.floor(Math.random() * 16)];
        }
        return color;
    }
});
