# component_wrappers.py

from playwright.sync_api import Page, Locator
from playwright._impl._errors import TimeoutError

def dump_visible_elements(page: Page):
    """Executes JS to find all visible, interactive selectors."""
    visible_selectors = page.evaluate(
        """() => {
            const visible = [];
            const interactiveTags = ['h1', 'h2', 'h3', 'p', 'button', 'input', 'a', 'div[id]', 'span[id]'];

            interactiveTags.forEach(tag => {
                const elements = document.querySelectorAll(tag);
                elements.forEach(el => {
                    const style = window.getComputedStyle(el);
                    // Check if element is visible and not placeholder text
                    if (style.display !== 'none' && style.visibility !== 'hidden' && style.opacity > 0) {
                        let text = el.innerText.substring(0, 40).replace(/\\n/g, '...');
                        let selector = el.id ? `#${el.id}` : el.tagName.toLowerCase();
                        visible.push({selector, text});
                    }
                });
            });
            return visible;
        }"""
    )

    if visible_selectors:
        for item in visible_selectors:
            print(f"[Visible] {item['selector']}: '{item['text']}'")
    else:
        print("No interactive elements found on the page.")


class Button:
    """
    A wrapper class for a Playwright Locator to simplify component interaction
    and property retrieval in tests.
    """
    def __init__(self, page: Page, selector: str, timeout=5.0):
        """
        Initializes the Button with a Playwright Locator.

        :param page: The Playwright Page object.
        :param selector: The CSS selector (e.g., "#main-button").
        """
        assert timeout > 0.0
        self._timeout = int(timeout * 1000)
        try:
            page.wait_for_selector(selector, state="visible", timeout=self._timeout)
        except TimeoutError:
            print(f"FAILURE: Timeout exceeded while waiting for: '{selector}'")
            dump_visible_elements(page) # Call the helper function
            raise

        self._locator = page.locator(selector)
        self.name = selector

    @property
    def classes(self) -> str:
        """
        Gets the computed background color of the button element.

        This property executes a JavaScript function in the browser to retrieve
        the final CSS value.
        """
        return self._locator.get_attribute("class").split(" ")

    def click(self):
        """Performs a simple click action on the locator."""
        self._locator.click()

    # You can add other properties here, like:
    @property
    def text(self) -> str:
        """Gets the text content of the button."""
        return self._locator.inner_text()
