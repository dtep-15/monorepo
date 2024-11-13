use leptos::{component, prelude::*, view, IntoView};
use web_sys::{wasm_bindgen::JsCast, HtmlElement, HtmlInputElement, SubmitEvent};

use crate::{api::{self, networks::{Network, NetworkConnection}}, set_app_state, web::{self, push_right}, AppState};

#[component]
pub fn configuration() -> impl IntoView {
	let networks = LocalResource::new(|| async move {
		api::retry(api::networks::networks).await
	});

	view! {
		<Suspense fallback=web::loading>
			{move || Suspend::new(async move { view! {
				<LoadedConfiguration networks={<Vec<Network> as Clone>::clone(&networks.await).into_iter()} />
			}})}
		</Suspense>
	}
}

#[component]
fn loaded_configuration(networks: impl Iterator<Item = Network>) -> impl IntoView {
	let switch_app_state = set_app_state();

	let on_connect = move |event: SubmitEvent| {
		let network_name = event
			.target().expect("no event target")
			.dyn_into::<HtmlElement>().unwrap()
			.get_attribute("data-network").expect("no data-network attribute")
			.into();

		let password = document().get_element_by_id(&format!("{}-password", network_name))
			.map(|input|
				input.dyn_into::<HtmlInputElement>()
					.expect("invalid password input element")
					.value()
					.into()
			);

		switch_app_state(AppState::ConfgurationConnecting(NetworkConnection {
			name: network_name,
			password,
		}));

		event.prevent_default();
	};
	
	view! {
		<h1>Connect to a network:</h1>
		<dt-network-list>
			{
				networks.map(|network|
					view! {
						<form data-network={network.name} on:submit={on_connect}>
							<dt-network>
								<h2>{network.name.clone()}</h2>
								{
									let input_id = format!("{}-password", network.name);

									if network.password_required {
										Some(view! {
											<label for={input_id.clone()}>
											Password:
											</label>
											<input id={input_id} class="network-password" type="password" required />
										})
									} else {
										None
									}
								}
								<dt-button-list>
									{push_right()}
									<button class="network-connect-btn" type="submit">Connect</button>
								</dt-button-list>
							</dt-network>
						</form>
					}).collect::<Vec<_>>()
			}
		</dt-network-list>
	}
}
