# JSON for voyage data, wxFileConfig for settings

Voyage data (accumulator state, voyage records, annual archives) is persisted as JSON files. Vessel Profile settings are stored via wxFileConfig in OpenCPN's shared configuration file (opencpn.conf / registry). JSON was chosen over SQLite to avoid a binary dependency for what is fundamentally a small dataset (a few hundred voyage records per year at most). JSON is human-readable, making debugging and manual data recovery straightforward. wxFileConfig for settings follows the established OpenCPN plugin convention — most plugins store their preferences this way, and OpenCPN's plugin API provides helper methods for configuration path management.

Considered alternatives:
- **SQLite**: Robust relational storage but overkill for the data volume and adds a compiled dependency.
- **XML**: OpenCPN's legacy format, but verbose and harder to work with programmatically than JSON.
- **wxFileConfig for everything**: Would require flattening structured voyage records into key-value pairs, losing the natural hierarchy.
