* This is an anonymous submission for the USENIX Security 2019 *

This repository provides essential pieces of the attacks we present in the paper.

Below is a description of the folders and files included in this repository:

Folder [Attacks]: The folder including 10 files describing our emulated attacks (M-1 to M-6 for multi-host and S-1 to S-4 for single-host). Each file gives detailed description about how the attack takes place step by step.

Folder [exfiltration_server]: Source code for the exfiltration server.

Folder [payloads]: Source code for all malicious payloads.

Folder [phishing_webpage]: The phishing webpage that the attacker used to achieve the lateral movement. In a multi-host attack scenario, the attacker replaces the original portal index with this phishing webpage. The implanted phishing webpage pretends that the user is required to download and install a necessary update. As a result, users who access the portal website will get convinced to download the malicious payload.

File [TC_Ground_Truth_Report_E3_Update.pdf]: DARPA Transparent Computing report, which describes different types attacks for engagement-style evaluation. All our emulated attacks are comparable to these attacks.

Please note that while these attacks are implemented with different CVEs and payload variants, they share a high-level similarity in terms of their patterns of those key attack-relevant events. The high-level, un-escapable similarity between the training and testing attacks is a key-requirement for ATLAS's investigation to succeed, as we discussed in the paper.